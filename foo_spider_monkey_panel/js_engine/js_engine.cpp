#include <stdafx.h>
#include "js_engine.h"

#include <js_engine/js_container.h>
#include <js_engine/js_compartment_inner.h>
#include <js_engine/js_internal_global.h>
#include <js_objects/global_object.h>
#include <js_utils/js_error_helper.h>
#include <utils/scope_helpers.h>
#include <utils/string_helpers.h>

#include <adv_config.h>
#include <heartbeat_window.h>
#include <host_timer_dispatcher.h>
#include <js_panel_window.h>
#include <popup_msg.h>
#include <user_message.h>

#include <js/Initialization.h>

using namespace smp;

namespace
{

constexpr uint32_t kHeartbeatRateMs = 73;
constexpr uint32_t kMonitorRateMs = 95;
constexpr uint32_t kJobsMaxBudgetMs = 500;

} // namespace

namespace
{

void ReportException( const pfc::string8_fast& errorText )
{
    const pfc::string8_fast errorTextPadded = [&errorText]() {
        pfc::string8_fast text = "Critical JS engine error: " SMP_NAME_WITH_VERSION;
        if ( !errorText.is_empty() )
        {
            text += "\n";
            text += errorText;
        }

        return text;
    }();

    FB2K_console_formatter() << errorTextPadded;
    popup_msg::g_show( errorTextPadded, SMP_NAME );
    MessageBeep( MB_ICONASTERISK );
}

} // namespace

namespace mozjs
{

JsEngine::JsEngine()
{
    JS_Init();
}

JsEngine::~JsEngine()
{ // Can't clean up here, since mozjs.dll might be already unloaded
    assert( !isInitialized_ );
}

JsEngine& JsEngine::GetInstance()
{
    static JsEngine je;
    return je;
}

void JsEngine::PrepareForExit()
{
    shouldShutdown_ = true;
}

bool JsEngine::RegisterContainer( JsContainer& jsContainer )
{
    if ( !registeredContainers_.size() && !Initialize() )
    {
        return false;
    }

    jsContainer.SetJsCtx( pJsCtx_ );

    assert( !registeredContainers_.count( &jsContainer ) );
    registeredContainers_.emplace( &jsContainer, jsContainer );

    return true;
}

void JsEngine::UnregisterContainer( JsContainer& jsContainer )
{
    if ( auto it = registeredContainers_.find( &jsContainer );
         it != registeredContainers_.end() )
    {
        it->second.get().Finalize();
        registeredContainers_.erase( it );
    }

    if ( !registeredContainers_.size() )
    {
        Finalize();
    }
}

void JsEngine::MaybeRunJobs()
{
    if ( !isInitialized_ )
    {
        return;
    }

    JSAutoRequest ar( pJsCtx_ );

    // TODO: add ability for user to abort script here

    if ( areJobsInProgress_ )
    { // might occur when called from nested loop
        /*
        // Use this only for script abort with error
        if ( timeGetTime() - jobsStartTime_ >= kJobsMaxBudgetMs )
        {
            js::StopDrainingJobQueue( pJsCtx_ );
        }
        */
        return;
    }

    jobsStartTime_ = timeGetTime();
    areJobsInProgress_ = true;

    {
        js::RunJobs( pJsCtx_ );

        for ( size_t i = 0; i < rejectedPromises_.length(); ++i )
        {
            const auto& rejectedPromise = rejectedPromises_[i];
            if ( !rejectedPromise )
            {
                continue;
            }

            JSAutoCompartment ac( pJsCtx_, rejectedPromise );
            mozjs::error::AutoJsReport are( pJsCtx_ );

            JS::RootedValue jsValue( pJsCtx_, JS::GetPromiseResult( rejectedPromise ) );
            if ( !jsValue.isNullOrUndefined() )
            {
                JS_SetPendingException( pJsCtx_, jsValue );
            }
            else
            { // Should not reach here, mostly paranoia check
                JS_ReportErrorUTF8( pJsCtx_, "Unhandled promise rejection" );
            }
        }
        rejectedPromises_.get().clear();
    }

    areJobsInProgress_ = false;
}

void JsEngine::OnJsActionStart( JsContainer& jsContainer )
{
    std::unique_lock<std::mutex> ul( monitorMutex_ );
    monitoredContainers_.emplace( &jsContainer, std::time( nullptr ) );
    hasMonitorAction_.notify_one();
}

void JsEngine::OnJsActionEnd( JsContainer& jsContainer )
{
    std::unique_lock<std::mutex> ul( monitorMutex_ );
    assert( monitoredContainers_.count( &jsContainer ) );
    monitoredContainers_.erase( &jsContainer );
}

const JsGc& JsEngine::GetGcEngine() const
{
    return jsGc_;
}

JsInternalGlobal& JsEngine::GetInternalGlobal()
{
    assert( internalGlobal_ );
    return *internalGlobal_;
}

void JsEngine::OnHeartbeat()
{
    if ( !isInitialized_ || isBeating_ || shouldStopHeartbeatThread_ )
    {
        return;
    }

    isBeating_ = true;

    {
        JSAutoRequest ar( pJsCtx_ );
        if ( !jsGc_.MaybeGc() )
        { // OOM
            ReportOomError();
        }
    }

    isBeating_ = false;
}

void JsEngine::OnModalWindowCreate()
{
    std::unique_lock<std::mutex> lock( monitorMutex_ );
    canProcessMonitor_ = false;
}

void JsEngine::OnModalWindowDestroy()
{
    const auto curTime = std::time( nullptr );

    std::unique_lock<std::mutex> lock( monitorMutex_ );
    for ( auto& [pContainer, startTime]: monitoredContainers_ )
    {
        startTime = curTime;
    }
    canProcessMonitor_ = true;
}

bool JsEngine::Initialize()
{
    if ( isInitialized_ )
    {
        return true;
    }

    utils::unique_ptr<JSContext> autoJsCtx( nullptr, []( auto pCtx ) {
        JS_DestroyContext( pCtx );
    } );

    try
    {
        autoJsCtx.reset( JS_NewContext( jsGc_.GetMaxHeap() ) );
        SmpException::ExpectTrue( autoJsCtx.get(), "JS_NewContext failed" );

        JSContext* cx = autoJsCtx.get();

        if ( !JS_AddInterruptCallback( cx, InterruptHandler ) )
        {
            throw smp::JsException();
        }

        if ( !js::UseInternalJobQueues( cx ) )
        {
            throw smp::JsException();
        }

        JS::SetPromiseRejectionTrackerCallback( cx, RejectedPromiseHandler, this );

        // TODO: JS::SetWarningReporter( pJsCtx_ )

        if ( !JS::InitSelfHostedCode( cx ) )
        {
            throw smp::JsException();
        }

        jsGc_.Initialize( cx );

        rejectedPromises_.init( cx, JS::GCVector<JSObject*, 0, js::SystemAllocPolicy>( js::SystemAllocPolicy() ) );

        internalGlobal_ = std::move( JsInternalGlobal::Create( cx ) );
        assert( internalGlobal_ );

        StartHeartbeatThread();
        StartMonitorThread();
    }
    catch ( const smp::JsException& )
    {
        assert( JS_IsExceptionPending( autoJsCtx.get() ) );
        ReportException( mozjs::error::JsErrorToText( autoJsCtx.get() ) );
        return false;
    }
    catch ( const SmpException& e )
    {
        ReportException( e.what() );
        return false;
    }

    pJsCtx_ = autoJsCtx.release();
    isInitialized_ = true;
    return true;
}

void JsEngine::Finalize()
{
    if ( pJsCtx_ )
    {
        StopMonitorThread();
        // Stop the thread first, so that we don't get additional GC's during jsGc.Finalize
        StopHeartbeatThread();
        jsGc_.Finalize();

        internalGlobal_.reset();
        rejectedPromises_.reset();

        JS_DestroyContext( pJsCtx_ );
        pJsCtx_ = nullptr;
    }

    if ( shouldShutdown_ )
    {
        HostTimerDispatcher::Get().Finalize();
        JS_ShutDown();
    }

    isInitialized_ = false;
}

void JsEngine::StartHeartbeatThread()
{
    if ( !heartbeatWindow_ )
    {
        heartbeatWindow_ = smp::HeartbeatWindow::Create();
        assert( heartbeatWindow_ );
    }

    shouldStopHeartbeatThread_ = false;
    heartbeatThread_ = std::thread( [parent = this] {
        while ( !parent->shouldStopHeartbeatThread_ )
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds( kHeartbeatRateMs ) );

            PostMessage( parent->heartbeatWindow_->GetHwnd(), static_cast<UINT>( smp::MiscMessage::heartbeat ), 0, 0 );
        }
    } );
}

void JsEngine::StopHeartbeatThread()
{
    if ( heartbeatThread_.joinable() )
    {
        shouldStopHeartbeatThread_ = true;
        heartbeatThread_.join();
    }
}

void JsEngine::StartMonitorThread()
{
    shouldStopMonitorThread_ = false;
    monitorThread_ = std::thread( [&] {
        decltype( slowContainers_ ) slowContainers;

        while ( !shouldStopMonitorThread_ )
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds( kMonitorRateMs ) );

            {
                std::unique_lock<std::mutex> lock( monitorMutex_ );
                hasMonitorAction_.wait( lock, [&] {
                    return shouldStopMonitorThread_ || !monitoredContainers_.empty() && canProcessMonitor_ && !isInInterrupt_;
                } );

                if ( shouldStopMonitorThread_ )
                {
                    break;
                }

                slowContainers.clear();
                const auto curTime = std::time( nullptr );
                for ( auto& [pContainer, startTime]: monitoredContainers_ )
                {
                    constexpr time_t maxDiff = 3;
                    if ( curTime - startTime > maxDiff )
                    {
                        slowContainers.emplace_back( pContainer );
                    }
                }
            }

            if ( !slowContainers.empty() )
            {
                {
                    std::unique_lock<std::mutex> lock( monitorMutex_ );
                    std::swap( slowContainers, slowContainers_ );
                }

                JS_RequestInterruptCallback( pJsCtx_ );
            }
        }
    } );
}

void JsEngine::StopMonitorThread()
{
    if ( monitorThread_.joinable() )
    {
        shouldStopMonitorThread_ = true;
        monitorThread_.join();
    }
}

bool JsEngine::InterruptHandler( JSContext* )
{
    return JsEngine::GetInstance().OnInterrupt();
}

bool JsEngine::OnInterrupt()
{
    if ( isInInterrupt_ )
    {
        return true;
    }
    isInInterrupt_ = true;
    smp::utils::final_action autoBool( [& isInInterrupt = isInInterrupt_] { isInInterrupt = false; } );

    decltype( slowContainers_ ) slowContainers;
    {
        std::unique_lock<std::mutex> lock( monitorMutex_ );
        if ( slowContainers_.empty() )
        {
            return true;
        }

        std::swap( slowContainers, slowContainers_ );
    }

    for ( auto& pContainer: slowContainers )
    {
        if ( !registeredContainers_.count( pContainer ) )
        {
            continue;
        }

        pContainer->Fail( "Script aborted by user" );
    }

    return false;
}

void JsEngine::RejectedPromiseHandler( JSContext* cx, JS::HandleObject promise, JS::PromiseRejectionHandlingState state, void* data )
{
    JsEngine& self = *reinterpret_cast<JsEngine*>( data );

    if ( JS::PromiseRejectionHandlingState::Handled == state )
    {
        auto& uncaughtRejections = self.rejectedPromises_;
        for ( size_t i = 0; i < uncaughtRejections.length(); ++i )
        {
            if ( uncaughtRejections[i] == promise )
            {
                // To avoid large amounts of memmoves, we don't shrink the vector here.
                // Instead, we filter out nullptrs when iterating over the vector later.
                uncaughtRejections[i].set( nullptr );
                break;
            }
        }
    }
    else
    {
        self.rejectedPromises_.get().append( promise );
    }
}

void JsEngine::ReportOomError()
{
    for ( auto& [hWnd, jsContainer]: registeredContainers_ )
    {
        auto& jsContainerRef = jsContainer.get();
        if ( mozjs::JsContainer::JsStatus::Working != jsContainerRef.GetStatus() )
        {
            continue;
        }

        jsContainerRef.Fail( fmt::format( "Out of memory: {}/{} bytes", jsContainerRef.pNativeCompartment_->GetCurrentHeapBytes(), jsGc_.GetMaxHeap() ).c_str() );
    }
}

} // namespace mozjs
