#pragma once

#include <convert/native_to_js.h>
#include <convert/js_to_native.h>
#include <js_utils/js_error_helper.h>

#include <optional>


namespace mozjs
{

template <typename ReturnType = std::nullptr_t, typename... ArgTypes>
std::optional<ReturnType> InvokeJsCallback( JSContext* cx,
                                            JS::HandleObject globalObject,
                                            pfc::string8_fast functionName,
                                            ArgTypes&&... args )
{
    assert( cx );
    assert( !!globalObject );
    assert( functionName.length() );

    JSAutoRequest ar( cx );
    JSAutoCompartment ac( cx, globalObject );
    AutoReportException are( cx );

    JS::RootedValue funcValue( cx );
    if ( !JS_GetProperty( cx, globalObject, functionName.c_str(), &funcValue ) )
    {// Reports
        return std::nullopt;
    }

    if ( funcValue.isUndefined() )
    {// Not an error: user does not handle the callback
        return nullptr;
    }

    JS::RootedValue retVal( cx );
    if constexpr ( sizeof...(ArgTypes) > 0 )
    {
        JS::AutoValueArray<sizeof...(ArgTypes)> wrappedArgs( cx );
        if ( !NativeToJsArguments( cx, wrappedArgs, 0, std::forward<ArgTypes>( args )... ) )
        {
            return std::nullopt;
        }

        if ( !InvokeJsCallback_Impl( cx, globalObject, funcValue, wrappedArgs, &retVal ) )
        {
            return std::nullopt;
        }
    }
    else
    {
        if ( !InvokeJsCallback_Impl( cx, globalObject, funcValue, JS::HandleValueArray::empty(), &retVal ) )
        {
            return std::nullopt;
        }
    }

    return convert::to_native::ToValue<ReturnType>( cx, retVal );
}

template <int ArgArraySize, typename ArgType, typename... ArgTypes>
bool NativeToJsArguments( JSContext * cx,
                          JS::AutoValueArray<ArgArraySize>& wrappedArgs,
                          uint8_t argIndex, ArgType&& arg, ArgTypes&&... args )
{
    return convert::to_js::ToValue( cx, std::forward<ArgType>( arg ), wrappedArgs[argIndex] )
        && NativeToJsArguments( cx, wrappedArgs, argIndex + 1, std::forward<ArgTypes>( args )... );
}

template <int ArgArraySize>
bool NativeToJsArguments( [[maybe_unused]] JSContext * cx,
                          [[maybe_unused]] JS::AutoValueArray<ArgArraySize>& wrappedArgs,
                          [[maybe_unused]] uint8_t argIndex )
{
    return true;
}

bool InvokeJsCallback_Impl( JSContext* cx,
                            JS::HandleObject globalObject,
                            JS::HandleValue functionValue,
                            const JS::HandleValueArray& args,
                            JS::MutableHandleValue rval );

}
