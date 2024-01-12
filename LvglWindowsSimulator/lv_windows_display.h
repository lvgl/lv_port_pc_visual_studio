/**
 * @file lv_windows_display.h
 *
 */

#ifndef LV_WINDOWS_DISPLAY_H
#define LV_WINDOWS_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"
#define LV_USE_WINDOWS

//#include "../../display/lv_display.h"
//#include "../../indev/lv_indev.h"

#ifdef LV_USE_WINDOWS

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

    lv_display_t* lv_windows_create_display(
        const wchar_t* title,
        int32_t hor_res,
        int32_t ver_res,
        int32_t zoom_level,
        bool allow_dpi_override,
        bool simulator_mode);

/**********************
 *      MACROS
 **********************/

#endif // LV_USE_WINDOWS

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_WINDOWS_DISPLAY_H*/
