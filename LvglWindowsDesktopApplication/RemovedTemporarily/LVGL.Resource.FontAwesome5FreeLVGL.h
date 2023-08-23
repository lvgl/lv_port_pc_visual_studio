/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Resource.FontAwesome5FreeLVGL.h
 * PURPOSE:   Definition for embedded resource - Font Awesome 5 Free LVGL
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#ifndef LVGL_RESOURCE_FONT_AWESOME_5_FREE_LVGL
#define LVGL_RESOURCE_FONT_AWESOME_5_FREE_LVGL

#include <stddef.h>
#include <stdint.h>

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C       extern "C"
#else
#define EXTERN_C       extern
#endif
#endif // !EXTERN_C

/**
 * @brief The resource of Font Awesome 5 Free LVGL font.
*/
EXTERN_C const uint8_t* LvglFontAwesome5FreeLvglFontResource;

/**
 * @brief The resource size of Font Awesome 5 Free LVGL font.
*/
EXTERN_C const size_t LvglFontAwesome5FreeLvglFontResourceSize;

/**
 * @brief The name of Font Awesome 5 Free LVGL font.
*/
EXTERN_C const wchar_t* LvglFontAwesome5FreeLvglFontName;

#endif // !LVGL_RESOURCE_FONT_AWESOME_5_FREE_LVGL
