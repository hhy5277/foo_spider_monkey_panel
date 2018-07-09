#include <stdafx.h>
#include "fb_properties.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_engine/native_to_js_converter.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>
#include <js_utils/serialized_value.h>
#include <js_utils/string_util.h>

#include <js_panel_window.h>

using namespace smp;

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JsFinalizeOp<JsFbProperties>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FbProperties",
    DefaultClassFlags(),
    &jsOps
};

const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

const JSPropertySpec jsProperties[] = {
    JS_PS_END
};

}

namespace mozjs
{

JsFbProperties::JsFbProperties( JSContext* cx, js_panel_window& parentPanel )
    : pJsCtx_( cx )
    , parentPanel_( parentPanel )
{
}

JsFbProperties::~JsFbProperties()
{
    RemoveHeapTracer();
}

JSObject* JsFbProperties::Create( JSContext* cx, js_panel_window& parentPanel )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties( cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    auto pNative = new JsFbProperties( cx, parentPanel );
    JS_SetPrivate( jsObj, pNative );

    if ( !JS_AddExtraGCRootsTracer( cx, JsFbProperties::TraceHeapValue, pNative ) )
    {
        return nullptr;
    }

    return jsObj;
}

const JSClass& JsFbProperties::GetClass()
{
    return jsClass;
}


void JsFbProperties::RemoveHeapTracer()
{
    JS_RemoveExtraGCRootsTracer( pJsCtx_, JsFbProperties::TraceHeapValue, this );
    properties_.clear();
}

std::optional<JS::Heap<JS::Value>>
JsFbProperties::GetProperty( const std::wstring& propName, JS::HandleValue propDefaultValue )
{    
    std::wstring trimmedPropName( Trim( propName ) );

    bool hasProperty = false;
    if ( properties_.count( trimmedPropName ) )
    {
        hasProperty = true;
    }
    else
    {
        auto prop = parentPanel_.get_config_prop().get_config_item( trimmedPropName );
        if ( prop )
        {
            JS::RootedValue jsProp( pJsCtx_ );
            if ( DeserializeJsValue( pJsCtx_, prop.value(), &jsProp ) )
            {
                hasProperty = true;
                properties_.emplace( trimmedPropName, std::make_shared<HeapElement>( jsProp ) );
            }
        }        
    }

    if ( !hasProperty )
    {
        if ( propDefaultValue.isNullOrUndefined() )
        {// Not a error: user does not want to set default value
            JS::Heap<JS::Value> tmpVal;
            tmpVal.setNull();
            return std::make_optional( tmpVal );
        }

        if ( !SetProperty( trimmedPropName, propDefaultValue ) )
        {
            return std::nullopt;
        }
    }

    return std::make_optional( properties_[trimmedPropName]->value );
}

bool JsFbProperties::SetProperty( const std::wstring& propName, JS::HandleValue propValue )
{
    std::wstring trimmedPropName( Trim( propName ) );

    if ( propValue.isNullOrUndefined() )
    {
        parentPanel_.get_config_prop().remove_config_item( trimmedPropName );
        properties_.erase( trimmedPropName );
        return true;
    }

    auto serializedValue = SerializeJsValue( pJsCtx_, propValue );
    if ( !serializedValue )
    {
        JS_ReportErrorUTF8( pJsCtx_, "Unsupported value type" );
        return false;
    }

    properties_[trimmedPropName] = std::make_shared<HeapElement>( propValue );
    parentPanel_.get_config_prop().set_config_item( trimmedPropName, serializedValue.value() );
    
    return true;
}

void JsFbProperties::TraceHeapValue( JSTracer *trc, void *data )
{
    assert( data );
    auto jsObject = static_cast<JsFbProperties*>( data );
    auto& properties = jsObject->properties_;
    
    for ( auto& elem : properties )
    {
        JS::TraceEdge( trc, &(elem.second->value), "CustomHeap_Properties" );
    }
}

}