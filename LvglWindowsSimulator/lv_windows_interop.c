/**
 * @file lv_windows_interop.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_windows_interop.h"
#ifdef LV_USE_WINDOWS

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

EXTERN_C HWND lv_windows_get_display_window_handle(
    lv_display_t* display)
{
    return (HWND)lv_display_get_driver_data(display);
}

EXTERN_C HWND lv_windows_get_indev_window_handle(
    lv_indev_t* indev)
{
    return lv_windows_get_display_window_handle(lv_indev_get_display(indev));
}

int32_t lv_windows_zoom_to_logical(int32_t physical, int32_t zoom_level)
{
    return MulDiv(physical, LV_WINDOWS_ZOOM_BASE_LEVEL, zoom_level);
}

int32_t lv_windows_zoom_to_physical(int32_t logical, int32_t zoom_level)
{
    return MulDiv(logical, zoom_level, LV_WINDOWS_ZOOM_BASE_LEVEL);
}

int32_t lv_windows_dpi_to_logical(int32_t physical, int32_t dpi)
{
    return MulDiv(physical, USER_DEFAULT_SCREEN_DPI, dpi);
}

int32_t lv_windows_dpi_to_physical(int32_t logical, int32_t dpi)
{
    return MulDiv(logical, dpi, USER_DEFAULT_SCREEN_DPI);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif // LV_USE_WINDOWS
