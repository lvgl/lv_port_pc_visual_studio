/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Windows.Font.h
 * PURPOSE:   Definition for Windows LVGL font utility functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#ifndef LVGL_WINDOWS_FONT
#define LVGL_WINDOWS_FONT

#include <Windows.h>

#if _MSC_VER >= 1200
// Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
#endif

#include "lvgl/lvgl.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C       extern "C"
#else
#define EXTERN_C       extern
#endif
#endif // !EXTERN_C

/**
 * @brief Default font for LVGL default theme.
*/

extern lv_font_t LvglDefaultFont;

/**
 * @brief Initialize the Windows GDI font engine.
 * @param FontName The default font name.
*/
EXTERN_C void WINAPI LvglWindowsGdiFontInitialize(
    _In_opt_ LPCWSTR FontName);

/**
 * @brief Creates a LVGL font object.
 * @param FontObject The LVGL font object.
 * @param FontSize The font size.
 * @param FontName The font name.
 * @return If succeed, return TRUE, otherwise return FALSE.
*/
EXTERN_C BOOL WINAPI LvglWindowsGdiFontCreateFont(
    _Out_ lv_font_t* FontObject,
    _In_ int FontSize,
    _In_opt_ LPCWSTR FontName);

#endif // !LVGL_WINDOWS_SYMBOL_FONT
