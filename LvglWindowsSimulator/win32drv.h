/**
 * @file win32drv.h
 *
 */

#ifndef LV_WINDOWS_DRV_H
#define LV_WINDOWS_DRV_H

/*********************
 *      INCLUDES
 *********************/

#include "lvgl/lvgl.h"

#include "lv_windows_interop.h"
#include "lv_windows_input.h"
#include "lv_windows_display.h"

#include <windows.h>

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

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C       extern "C"
#else
#define EXTERN_C       extern
#endif
#endif // !EXTERN_C

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

EXTERN_C bool lv_windows_platform_init();

/**********************
 *      MACROS
 **********************/

#endif /*LV_WINDOWS_DRV_H*/
