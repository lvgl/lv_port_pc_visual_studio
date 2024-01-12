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
