/**
 * @file win32drv.h
 *
 */

#ifndef LV_WIN32DRV_H
#define LV_WIN32DRV_H

/*********************
 *      INCLUDES
 *********************/

#include <Windows.h>

#if _MSC_VER >= 1200
 // Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

EXTERN_C bool lv_win32_quit_signal;

EXTERN_C bool lv_win32_init(
    HINSTANCE instance_handle,
    int show_window_mode,
    lv_coord_t hor_res,
    lv_coord_t ver_res,
    HICON icon_handle);

/**********************
 *      MACROS
 **********************/

#endif /*LV_WIN32DRV_H*/
