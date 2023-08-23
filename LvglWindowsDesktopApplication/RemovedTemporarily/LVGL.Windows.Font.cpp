/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Windows.Font.cpp
 * PURPOSE:   Implementation for Windows LVGL font utility functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include "LVGL.Windows.Font.h"

#include "LVGL.Resource.FontAwesome5Free.h"
#include "LVGL.Resource.FontAwesome5FreeLVGL.h"

#include <cstdint>
#include <map>
#include <vector>

static FIXED ConvertDoubleToFixed(double value)
{
    std::uint32_t result = static_cast<std::uint32_t>(value * (1 << 16));
    return *reinterpret_cast<FIXED*>(&result);
}

lv_font_t LvglDefaultFont;

typedef struct _LVGL_WINDOWS_GDI_FONT_CONTEXT
{
    HFONT FontHandle;
    HFONT SymbolFontHandle;
    using GlyphValueType = std::pair<lv_font_glyph_dsc_t, std::size_t>;
    std::map<std::uint32_t, GlyphValueType> GlyphSet;
    std::uint32_t DpiValue;
    std::vector<std::uint8_t> GlyphBitmapPool;
} LVGL_WINDOWS_GDI_FONT_CONTEXT, *PLVGL_WINDOWS_GDI_FONT_CONTEXT;

static void LvglWindowsGdiFontAddGlyph(
    PLVGL_WINDOWS_GDI_FONT_CONTEXT Context,
    std::uint32_t UnicodeLetter)
{
    MAT2 TransformationMatrix;
    TransformationMatrix.eM11 = ::ConvertDoubleToFixed(
        static_cast<double>(Context->DpiValue) / USER_DEFAULT_SCREEN_DPI);
    TransformationMatrix.eM12= ::ConvertDoubleToFixed(
        0.0);
    TransformationMatrix.eM21= ::ConvertDoubleToFixed(
        0.0);
    TransformationMatrix.eM22 = ::ConvertDoubleToFixed(
        static_cast<double>(Context->DpiValue) / USER_DEFAULT_SCREEN_DPI);

    HDC ContextDCHandle = nullptr;

    do
    {
        ContextDCHandle = ::GetDC(nullptr);
        if (!ContextDCHandle)
        {
            break;
        }

        wchar_t InBuffer[2];
        InBuffer[0] = static_cast<wchar_t>(UnicodeLetter);
        InBuffer[1] = L'\0';

        WORD OutBuffer[2];
        OutBuffer[0] = 0;
        OutBuffer[1] = 0;

        ::SelectObject(ContextDCHandle, Context->FontHandle);
        if (::GetGlyphIndicesW(
            ContextDCHandle,
            InBuffer,
            1,
            OutBuffer,
            GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR
            || OutBuffer[0] == 0xffff)
        {
            ::SelectObject(ContextDCHandle, Context->SymbolFontHandle);
            if (::GetGlyphIndicesW(
                ContextDCHandle,
                InBuffer,
                1,
                OutBuffer,
                GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR
                || OutBuffer[0] == 0xffff)
            {
                break;
            }
        }

        GLYPHMETRICS GlyphMetrics;
        std::size_t GlyphBitmapOffset = static_cast<std::size_t>(-1);
        DWORD Length = ::GetGlyphOutlineW(
            ContextDCHandle,
            OutBuffer[0],
            GGO_GRAY8_BITMAP | GGO_GLYPH_INDEX,
            &GlyphMetrics,
            0,
            nullptr,
            &TransformationMatrix);
        if (Length != GDI_ERROR)
        {
            if (Length > 0)
            {
                GlyphBitmapOffset = Context->GlyphBitmapPool.size();
                Context->GlyphBitmapPool.resize(GlyphBitmapOffset + Length);
                uint8_t* GlyphBitmap = &Context->GlyphBitmapPool[GlyphBitmapOffset];

                if (::GetGlyphOutlineW(
                    ContextDCHandle,
                    OutBuffer[0],
                    GGO_GRAY8_BITMAP | GGO_GLYPH_INDEX,
                    &GlyphMetrics,
                    Length,
                    GlyphBitmap,
                    &TransformationMatrix) != GDI_ERROR)
                {
                    for (size_t i = 0; i < Length; ++i)
                    {
                        GlyphBitmap[i] =
                            GlyphBitmap[i] == 0x40
                            ? 0xFF
                            : GlyphBitmap[i] << 2;
                    }
                }
            }

            lv_font_glyph_dsc_t GlyphDescription;
            GlyphDescription.adv_w = GlyphMetrics.gmCellIncX;
            GlyphDescription.box_w = static_cast<std::uint16_t>(
                (GlyphMetrics.gmBlackBoxX + 0x0003) & 0xFFFC);
            GlyphDescription.box_h = static_cast<std::uint16_t>(
                GlyphMetrics.gmBlackBoxY);
            GlyphDescription.ofs_x = static_cast<std::int16_t>(
                GlyphMetrics.gmptGlyphOrigin.x);
            GlyphDescription.ofs_y = static_cast<std::int16_t>(
                GlyphMetrics.gmptGlyphOrigin.y - GlyphMetrics.gmBlackBoxY);
            GlyphDescription.bpp = 8;

            Context->GlyphSet.emplace(std::make_pair(
                UnicodeLetter,
                std::make_pair(GlyphDescription, GlyphBitmapOffset)));
        }

    } while (false);

    if (ContextDCHandle)
    {
        ::ReleaseDC(nullptr, ContextDCHandle);
    }
}

static void LvglWindowsGdiFontCheckDpiValue(
    PLVGL_WINDOWS_GDI_FONT_CONTEXT Context)
{
    std::uint32_t DpiValue = LV_DPI_DEF;
    lv_disp_t* CurrentDisplay = ::lv_disp_get_default();
    if (CurrentDisplay)
    {
        DpiValue = ::lv_disp_get_dpi(CurrentDisplay);
    }

    if (DpiValue != Context->DpiValue)
    {
        Context->GlyphSet.clear();
        Context->GlyphBitmapPool.clear();
        Context->DpiValue = DpiValue;
    }
}

static bool LvglWindowsGdiFontGetGlyphCallback(
    const lv_font_t* font,
    lv_font_glyph_dsc_t* dsc_out,
    uint32_t unicode_letter,
    uint32_t unicode_letter_next)
{
    unicode_letter_next;

    PLVGL_WINDOWS_GDI_FONT_CONTEXT Context =
        reinterpret_cast<PLVGL_WINDOWS_GDI_FONT_CONTEXT>(
            const_cast<void*>(font->dsc));

    ::LvglWindowsGdiFontCheckDpiValue(Context);

    auto iterator = Context->GlyphSet.find(unicode_letter);
    if (iterator == Context->GlyphSet.end())
    {
        ::LvglWindowsGdiFontAddGlyph(
            Context,
            unicode_letter);

        iterator = Context->GlyphSet.find(unicode_letter);
        if (iterator == Context->GlyphSet.end())
        {
            return false;
        }
    }

    std::memcpy(
        dsc_out,
        &iterator->second.first,
        sizeof(lv_font_glyph_dsc_t));

    return true;
}

static const uint8_t* LvglWindowsGdiFontGetGlyphBitmapCallback(
    const lv_font_t* font,
    uint32_t unicode_letter)
{
    PLVGL_WINDOWS_GDI_FONT_CONTEXT Context =
        reinterpret_cast<PLVGL_WINDOWS_GDI_FONT_CONTEXT>(
            const_cast<void*>(font->dsc));

    ::LvglWindowsGdiFontCheckDpiValue(Context);

    auto iterator = Context->GlyphSet.find(unicode_letter);
    if (iterator == Context->GlyphSet.end())
    {
        ::LvglWindowsGdiFontAddGlyph(
            Context,
            unicode_letter);

        iterator = Context->GlyphSet.find(unicode_letter);
        if (iterator == Context->GlyphSet.end())
        {
            return nullptr;
        }
    }

    std::size_t& GlyphBitmapOffset = iterator->second.second;
    return
        (GlyphBitmapOffset != static_cast<std::size_t>(-1))
        ? &Context->GlyphBitmapPool[GlyphBitmapOffset]
        : nullptr;
}

EXTERN_C void WINAPI LvglWindowsGdiFontInitialize(
    _In_opt_ LPCWSTR FontName)
{
    DWORD NumFonts = 0;
    ::AddFontMemResourceEx(
        const_cast<uint8_t*>(LvglFontAwesome5FreeLvglFontResource),
        static_cast<DWORD>(LvglFontAwesome5FreeLvglFontResourceSize),
        nullptr,
        &NumFonts);

    ::LvglWindowsGdiFontCreateFont(
        &LvglDefaultFont,
        16,
        FontName);
}

EXTERN_C BOOL WINAPI LvglWindowsGdiFontCreateFont(
    _Out_ lv_font_t* FontObject,
    _In_ int FontSize,
    _In_opt_ LPCWSTR FontName)
{
    HFONT FontHandle = nullptr;
    HFONT SymbolFontHandle = nullptr;

    PLVGL_WINDOWS_GDI_FONT_CONTEXT Context = nullptr;

    do
    {
        FontHandle = ::CreateFontW(
            -FontSize,                  // nHeight
            0,                         // nWidth
            0,                         // nEscapement
            0,                         // nOrientation
            FW_NORMAL,                 // nWeight
            FALSE,                     // bItalic
            FALSE,                     // bUnderline
            0,                         // cStrikeOut
            DEFAULT_CHARSET,           // nCharSet
            OUT_DEFAULT_PRECIS,        // nOutPrecision
            CLIP_DEFAULT_PRECIS,       // nClipPrecision
            CLEARTYPE_NATURAL_QUALITY, // nQuality
            FF_DONTCARE,  // nPitchAndFamily
            FontName);
        if (!FontHandle)
        {
            break;
        }

        SymbolFontHandle = ::CreateFontW(
            -FontSize,                  // nHeight
            0,                         // nWidth
            0,                         // nEscapement
            0,                         // nOrientation
            FW_NORMAL,                 // nWeight
            FALSE,                     // bItalic
            FALSE,                     // bUnderline
            0,                         // cStrikeOut
            DEFAULT_CHARSET,           // nCharSet
            OUT_DEFAULT_PRECIS,        // nOutPrecision
            CLIP_DEFAULT_PRECIS,       // nClipPrecision
            CLEARTYPE_NATURAL_QUALITY, // nQuality
            FF_DONTCARE,  // nPitchAndFamily
            LvglFontAwesome5FreeLvglFontName);
        if (!FontHandle)
        {
            break;
        }

        std::uint32_t DpiValue = LV_DPI_DEF;
        lv_disp_t* CurrentDisplay = ::lv_disp_get_default();
        if (CurrentDisplay)
        {
            DpiValue = ::lv_disp_get_dpi(CurrentDisplay);
        }

        Context = new LVGL_WINDOWS_GDI_FONT_CONTEXT();
        if (!Context)
        {
            break;
        }

        FontObject->dsc = Context;

        Context->FontHandle = FontHandle;
        Context->SymbolFontHandle = SymbolFontHandle;

        Context->DpiValue = DpiValue;

        bool IsSucceed = false;
        HDC ContextDCHandle = ::GetDC(nullptr);
        if (ContextDCHandle)
        {
            ::SelectObject(ContextDCHandle, Context->FontHandle);

            OUTLINETEXTMETRICW OutlineTextMetrics;
            if (::GetOutlineTextMetricsW(
                ContextDCHandle,
                sizeof(OUTLINETEXTMETRICW),
                &OutlineTextMetrics))
            {
                FontObject->get_glyph_dsc =
                    ::LvglWindowsGdiFontGetGlyphCallback;
                FontObject->get_glyph_bitmap =
                    ::LvglWindowsGdiFontGetGlyphBitmapCallback;
                FontObject->line_height = static_cast<lv_coord_t>(
                    OutlineTextMetrics.otmLineGap
                    - OutlineTextMetrics.otmDescent
                    + OutlineTextMetrics.otmAscent);
                FontObject->base_line = static_cast<lv_coord_t>(
                    0 - OutlineTextMetrics.otmDescent);
                FontObject->subpx = LV_FONT_SUBPX_NONE;
                FontObject->underline_position = static_cast<std::int8_t>(
                    OutlineTextMetrics.otmsUnderscorePosition);
                FontObject->underline_thickness = static_cast<std::int8_t>(
                    OutlineTextMetrics.otmsUnderscoreSize);

                IsSucceed = true;
            }

            ::ReleaseDC(nullptr, ContextDCHandle);
        }

        if (!IsSucceed)
        {
            break;
        }

        return TRUE;

    } while (false);

    if (Context)
    {
        delete Context;
    }

    if (SymbolFontHandle)
    {
        ::DeleteObject(SymbolFontHandle);
    }

    if (FontHandle)
    {
        ::DeleteObject(FontHandle);
    }

    return FALSE;
}
