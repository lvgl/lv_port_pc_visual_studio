/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Resource.FontAwesome5Free.h
 * PURPOSE:   Definition for embedded resource - Font Awesome 5 Free
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#ifndef LVGL_RESOURCE_FONT_AWESOME_5_FREE
#define LVGL_RESOURCE_FONT_AWESOME_5_FREE

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
 * @brief The resource of Font Awesome 5 Free font.
*/
EXTERN_C const uint8_t* LvglFontAwesome5FreeFontResource;

/**
 * @brief The resource size of Font Awesome 5 Free font.
*/
EXTERN_C const size_t LvglFontAwesome5FreeFontResourceSize;

/**
 * @brief The name of Font Awesome 5 Free font.
*/
EXTERN_C const wchar_t* LvglFontAwesome5FreeFontName;

#endif // !LVGL_RESOURCE_FONT_AWESOME_5_FREE
