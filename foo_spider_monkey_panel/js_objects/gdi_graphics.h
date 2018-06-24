#pragma once

#include <js_engine/js_error_codes.h>
#include <js_objects/js_object_wrapper.h>

#include <optional>

class JSObject;
struct JSContext;

namespace Gdi
{
class Graphics;
}

namespace mozjs
{

class JsGdiFont;

class JsGdiGraphics
{
public:
    ~JsGdiGraphics();

    static JSObject* Create( JSContext* cx );

    void SetGraphicsObject( Gdiplus::Graphics* graphics );

public:
    std::optional<uint32_t> CalcTextHeight( std::wstring str, JsGdiFont* pJsFont );
    std::optional<uint32_t> CalcTextWidth( std::wstring str, JsGdiFont* pJsFont );
    std::optional<std::nullptr_t> DrawEllipse( float x, float y, float w, float h, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawLine( float x1, float y1, float x2, float y2, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawPolygon( uint32_t colour, float line_width, std::vector<JsUnknownObjectWrapper> points );
    std::optional<std::nullptr_t> DrawRect( float x, float y, float w, float h, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, float line_width, uint32_t colour );
    std::optional<std::nullptr_t> DrawString( std::wstring str, JsGdiFont* pJsFont, uint32_t colour, float x, float y, float w, float h, uint32_t flags );
    std::optional<std::nullptr_t> DrawStringWithOpt( size_t optArgCount, std::wstring str, JsGdiFont* pJsFont, uint32_t colour, float x, float y, float w, float h, uint32_t flags );
    std::optional<std::nullptr_t> FillEllipse( float x, float y, float w, float h, uint32_t colour );
    std::optional<std::nullptr_t> FillGradRect( float x, float y, float w, float h, float angle, uint32_t colour1, uint32_t colour2, float focus );
    std::optional<std::nullptr_t> FillPolygon( uint32_t colour, uint32_t fillmode, std::vector<JsUnknownObjectWrapper> points );
    std::optional<std::nullptr_t> FillRoundRect( float x, float y, float w, float h, float arc_width, float arc_height, uint32_t colour );
    std::optional<std::nullptr_t> FillSolidRect( float x, float y, float w, float h, uint32_t colour );
    std::optional<std::nullptr_t> SetInterpolationMode( uint32_t mode );
    std::optional<std::nullptr_t> SetInterpolationModeWithOpt( size_t optArgCount, uint32_t mode );
    std::optional<std::nullptr_t> SetSmoothingMode( uint32_t mode );
    std::optional<std::nullptr_t> SetSmoothingModeWithOpt( size_t optArgCount, uint32_t mode );
    std::optional<std::nullptr_t> SetTextRenderingHint( uint32_t mode );
    std::optional<std::nullptr_t> SetTextRenderingHintWithOpt( size_t optArgCount, uint32_t mode );

    //DrawImage( IGdiBitmap* image, float dstX, float dstY, float dstW, float dstH, float srcX, float srcY, float srcW, float srcH, float angle, BYTE alpha );


    //EstimateLineWrap( BSTR str, IGdiFont* font, int max_width, VARIANT* p );

    //GdiAlphaBlend( IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH, BYTE alpha );
    //GdiDrawBitmap( IGdiRawBitmap* bitmap, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH );
    //GdiDrawText( BSTR str, IGdiFont* font, VARIANT colour, int x, int y, int w, int h, int format, VARIANT* p );
    //MeasureString( BSTR str, IGdiFont* font, float x, float y, float w, float h, int flags, IMeasureStringInfo** pp );


private:
    JsGdiGraphics( JSContext* cx );
    JsGdiGraphics( const JsGdiGraphics& ) = delete;

    bool GetRoundRectPath( Gdiplus::GraphicsPath& gp, Gdiplus::RectF& rect, float arc_width, float arc_height );
    bool ParsePoints( std::vector<JsUnknownObjectWrapper> jsPoints, std::vector<Gdiplus::PointF> &gdiPoints );

private:
    JSContext * pJsCtx_;
    Gdiplus::Graphics* graphics_;
};

}