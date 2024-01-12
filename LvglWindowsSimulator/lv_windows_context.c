/**
 * @file lv_windows_context.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_windows_context.h"
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

lv_windows_window_context_t* lv_windows_get_window_context(
    HWND window_handle)
{
    return (lv_windows_window_context_t*)(
        GetPropW(window_handle, L"LVGL.Window.Context"));
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif // LV_USE_WINDOWS
