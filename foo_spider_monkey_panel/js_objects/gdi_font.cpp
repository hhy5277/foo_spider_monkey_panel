#include <stdafx.h>

#include "gdi_font.h"

#include <js_engine/js_value_converter.h>
#include <js_engine/js_native_invoker.h>
#include <js_engine/js_error_reporter.h>
#include <js_objects/gdi_error.h>


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
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "GdiFont",
    JSCLASS_HAS_PRIVATE,
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Height )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Name )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Size )
MJS_DEFINE_JS_TO_NATIVE_FN( JsGdiFont, Style )

const JSPropertySpec jsProperties[] = {
    JS_PSG( "Height", Height, 0 ),
    JS_PSG( "Name", Name, 0 ),
    JS_PSG( "Size", Size, 0 ),
    JS_PSG( "Style", Style, 0 ),
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {
    JS_FS_END
};

}

namespace mozjs
{


JsGdiFont::JsGdiFont( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont )
    : pJsCtx_( cx )
    , gdiFont_( pGdiFont )
    , hFont_( hFont )
{
}


JsGdiFont::~JsGdiFont()
{
}
// TODO: implement isManaged
JSObject* JsGdiFont::Create( JSContext* cx, Gdiplus::Font* pGdiFont, HFONT hFont, bool isManaged )
{
    JS::RootedObject jsObj( cx,
                            JS_NewObject( cx, &jsClass ) );
    if ( !jsObj )
    {
        return nullptr;
    }

    if ( !JS_DefineFunctions( cx, jsObj, jsFunctions )
         || !JS_DefineProperties(cx, jsObj, jsProperties ) )
    {
        return nullptr;
    }

    JS_SetPrivate( jsObj, new JsGdiFont( cx, pGdiFont, hFont ) );

    return jsObj;
}

const JSClass& JsGdiFont::GetClass()
{
    return jsClass;
}

Gdiplus::Font* JsGdiFont::GdiFont() const
{
    return gdiFont_.get();
}

HFONT JsGdiFont::HFont() const
{
    return hFont_;
}

std::optional<uint32_t>
JsGdiFont::Height() const
{
    if ( !gdiFont_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    Gdiplus::Bitmap img( 1, 1, PixelFormat32bppPARGB );
    Gdiplus::Graphics g( &img );

    return std::optional<uint32_t>{static_cast<uint32_t>(gdiFont_->GetHeight( &g ))};
}

std::optional<std::wstring>
JsGdiFont::Name() const
{
    if ( !gdiFont_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    Gdiplus::FontFamily fontFamily;
    WCHAR name[LF_FACESIZE] = { 0 };
    Gdiplus::Status gdiRet = gdiFont_->GetFamily( &fontFamily );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, GetFamily );

    gdiRet = fontFamily.GetFamilyName( name, LANG_NEUTRAL );
    IF_GDI_FAILED_RETURN_WITH_REPORT( pJsCtx_, gdiRet, std::nullopt, GetFamilyName );
    
    return std::optional<std::wstring>{name};
}

std::optional<float>
JsGdiFont::Size() const
{
    if ( !gdiFont_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    return std::optional<float>{gdiFont_->GetSize()};
}

std::optional<uint32_t>
JsGdiFont::Style() const
{
    if ( !gdiFont_ )
    {
        JS_ReportErrorASCII( pJsCtx_, "Internal error: Gdiplus::Font object is null" );
        return std::nullopt;
    }

    return std::optional<uint32_t>{gdiFont_->GetStyle()};
}

}