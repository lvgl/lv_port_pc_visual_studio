/*
 * PROJECT:   LVGL PC Simulator using Visual Studio
 * FILE:      LVGL.Simulator.cpp
 * PURPOSE:   Implementation for LVGL ported to Windows Desktop
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include <Windows.h>

#include "resource.h"

#if _MSC_VER >= 1200
 // Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
#endif

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/win32drv/win32drv.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

#include <stdio.h>

bool single_display_mode_initialization()
{
    if (!lv_win32_init(
        GetModuleHandleW(NULL),
        SW_SHOW,
        800,
        480,
        LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_LVGL))))
    {
        return false;
    }

    lv_win32_add_all_input_devices_to_group(NULL);

    return true;
}

#include <process.h>

HANDLE g_window_mutex = NULL;
bool g_initialization_status = false;

#define LVGL_SIMULATOR_MAXIMUM_DISPLAYS 16
HWND g_display_window_handles[LVGL_SIMULATOR_MAXIMUM_DISPLAYS];

unsigned int __stdcall lv_win32_window_thread_entrypoint(
    void* raw_parameter)
{
    size_t display_id = *(size_t*)(raw_parameter);

    HINSTANCE instance_handle = GetModuleHandleW(NULL);
    int show_window_mode = SW_SHOW;
    HICON icon_handle = LoadIconW(instance_handle, MAKEINTRESOURCE(IDI_LVGL));
    lv_coord_t hor_res = 800;
    lv_coord_t ver_res = 450;

    wchar_t window_title[256];
    memset(window_title, 0, sizeof(window_title));
    _snwprintf(
        window_title,
        256,
        L"LVGL Simulator for Windows Desktop (Display %d)",
        display_id);

    g_display_window_handles[display_id] = lv_win32_create_display_window(
        window_title,
        hor_res,
        ver_res,
        instance_handle,
        icon_handle,
        show_window_mode);
    if (!g_display_window_handles[display_id])
    {
        return 0;
    }

    g_initialization_status = true;

    SetEvent(g_window_mutex);

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    lv_win32_quit_signal = true;

    return 0;
}

bool multiple_display_mode_initialization()
{
    if (!lv_win32_init_window_class())
    {
        return false;
    }

    for (size_t i = 0; i < LVGL_SIMULATOR_MAXIMUM_DISPLAYS; ++i)
    {
        g_initialization_status = false;

        g_window_mutex = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);

        _beginthreadex(
            NULL,
            0,
            lv_win32_window_thread_entrypoint,
            &i,
            0,
            NULL);

        WaitForSingleObjectEx(g_window_mutex, INFINITE, FALSE);

        CloseHandle(g_window_mutex);

        if (!g_initialization_status)
        {
            return false;
        }
    }

    lv_win32_window_context_t* context = (lv_win32_window_context_t*)(
        lv_win32_get_window_context(g_display_window_handles[0]));
    if (context)
    {
        lv_win32_pointer_device_object = context->mouse_device_object;
        lv_win32_keypad_device_object = context->keyboard_device_object;
        lv_win32_encoder_device_object = context->mousewheel_device_object;
    }

    lv_win32_add_all_input_devices_to_group(NULL);

    return true;
}

//static lv_obj_t* label;
//
//static void slider_event_cb(lv_event_t* e)
//{
//    lv_obj_t* slider = lv_event_get_target(e);
//
//    /*Refresh the text*/
//    lv_label_set_text_fmt(label, "%d", lv_slider_get_value(slider));
//    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);    /*Align top of the slider*/
//}

int main()
{
    lv_init();

    if (!single_display_mode_initialization())
    {
        return -1;
    }

    /*if (!multiple_display_mode_initialization())
    {
        return -1;
    }
    else
    {
        for (size_t i = 0; i < LVGL_SIMULATOR_MAXIMUM_DISPLAYS; ++i)
        {
            lv_win32_window_context_t* context = (lv_win32_window_context_t*)(
                lv_win32_get_window_context(g_display_window_handles[i]));
            if (context)
            {
                lv_disp_set_default(context->display_device_object);
                switch (i)
                {
                case 0:
                    lv_demo_widgets();
                    break;
                case 1:
                    lv_demo_benchmark();
                    break;
                case 2:
                    lv_example_style_1();
                    break;
                case 3:
                    lv_example_get_started_1();
                    break;
                case 4:
                    lv_example_anim_1();
                    break;
                case 5:
                    lv_example_style_2();
                    break;
                case 6:
                    lv_example_get_started_2();
                    break;
                case 7:
                    lv_example_anim_2();
                    break;
                case 8:
                    lv_example_style_3();
                    break;
                case 9:
                    lv_example_get_started_3();
                    break;
                case 10:
                    lv_example_anim_3();
                    break;
                case 11:
                    lv_example_style_4();
                    break;
                case 12:
                    lv_example_style_5();
                    break;
                case 13:
                    lv_example_style_6();
                    break;
                case 14:
                    lv_example_imgfont_1();
                    break;
                case 15:
                    lv_example_style_7();
                    break;
                default:
                    break;
                }
            }
        }
    }*/

    //lv_win32_window_context_t* context = (lv_win32_window_context_t*)(
    //    lv_win32_get_window_context(g_display_window_handles[1]));
    //if (context)
    //{
    //    lv_obj_t* scr = lv_disp_get_scr_act(context->display_device_object);

    //    /*Create a slider in the center of the display*/
    //    lv_obj_t* slider = lv_slider_create(scr);
    //    lv_obj_set_width(slider, 200);                          /*Set the width*/
    //    lv_obj_center(slider);                                  /*Align to the center of the parent (screen)*/
    //    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);     /*Assign an event function*/

    //    /*Create a label above the slider*/
    //    label = lv_label_create(scr);
    //    lv_label_set_text(label, "0");
    //    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15);    /*Align top of the slider*/
    //}

    /*
     * Demos, benchmarks, and tests.
     *
     * Uncomment any one (and only one) of the functions below to run that
     * item.
     */

    // ----------------------------------
    // my freetype application
    // ----------------------------------

    ///*Init freetype library
    // *Cache max 64 faces and 1 size*/
    //lv_freetype_init(64, 1, 0);

    ///*Create a font*/
    //static lv_ft_info_t info;
    //info.name = "./lvgl/src/extra/libs/freetype/arial.ttf";
    //info.weight = 36;
    //info.style = FT_FONT_STYLE_NORMAL;
    //lv_ft_font_init(&info);

    ///*Create style with the new font*/
    //static lv_style_t style;
    //lv_style_init(&style);
    //lv_style_set_text_font(&style, info.font);

    ///*Create a label with the new style*/
    //lv_obj_t* label = lv_label_create(lv_scr_act());
    //lv_obj_add_style(label, &style, 0);
    //lv_label_set_text(label, "FreeType Arial Test");

    // ----------------------------------
    // my Win32 filesystem driver application
    // ----------------------------------

    /*::lv_fs_win32_init();

    lv_fs_dir_t d;
    if (lv_fs_dir_open(&d, "/") == LV_FS_RES_OK)
    {
        char b[MAX_PATH];
        memset(b, 0, MAX_PATH);
        while (lv_fs_dir_read(&d, b) == LV_FS_RES_OK)
        {
            printf("%s\n", b);
        }

        lv_fs_dir_close(&d);
    }*/

    // ----------------------------------
    // Demos from lv_examples
    // ----------------------------------

    lv_demo_widgets();           // ok
    //lv_demo_benchmark();
    // lv_demo_keypad_encoder();    // ok
    // lv_demo_music();             // removed from repository
    // lv_demo_printer();           // removed from repository
    // lv_demo_stress();            // ok

    // ----------------------------------
    // LVGL examples
    // ----------------------------------

    /*
     * There are many examples of individual widgets found under the
     * lvgl\exampless directory.  Here are a few sample test functions.
     * Look in that directory to find all the rest.
     */

    // lv_ex_get_started_1();
    // lv_ex_get_started_2();
    // lv_ex_get_started_3();

    // lv_example_flex_1();
    // lv_example_flex_2();
    // lv_example_flex_3();
    // lv_example_flex_4();
    // lv_example_flex_5();
    // lv_example_flex_6();        // ok

    // lv_example_grid_1();
    // lv_example_grid_2();
    // lv_example_grid_3();
    // lv_example_grid_4();
    // lv_example_grid_5();
    // lv_example_grid_6();

    // lv_port_disp_template();
    // lv_port_fs_template();
    // lv_port_indev_template();

    // lv_example_scroll_1();
    // lv_example_scroll_2();
    // lv_example_scroll_3();

    // lv_example_style_1();
    // lv_example_style_2();
    // lv_example_style_3();
    // lv_example_style_4();        // ok
    // lv_example_style_6();        // file has no source code
    // lv_example_style_7();
    // lv_example_style_8();
    // lv_example_style_9();
    // lv_example_style_10();
    // lv_example_style_11();       // ok

    // ----------------------------------
    // LVGL widgets examples
    // ----------------------------------

    // lv_example_arc_1();
    // lv_example_arc_2();

    // lv_example_bar_1();          // ok
    // lv_example_bar_2();
    // lv_example_bar_3();
    // lv_example_bar_4();
    // lv_example_bar_5();
    // lv_example_bar_6();          // issues

    // lv_example_btn_1();
    // lv_example_btn_2();
    // lv_example_btn_3();

    // lv_example_btnmatrix_1();
    // lv_example_btnmatrix_2();
    // lv_example_btnmatrix_3();

    // lv_example_calendar_1();

    // lv_example_canvas_1();
    // lv_example_canvas_2();

    // lv_example_chart_1();        // ok
    // lv_example_chart_2();        // ok
    // lv_example_chart_3();        // ok
    // lv_example_chart_4();        // ok
    // lv_example_chart_5();        // ok
    // lv_example_chart_6();        // ok

    // lv_example_checkbox_1();

    // lv_example_colorwheel_1();   // ok

    // lv_example_dropdown_1();
    // lv_example_dropdown_2();
    // lv_example_dropdown_3();

    // lv_example_img_1();
    // lv_example_img_2();
    // lv_example_img_3();
    // lv_example_img_4();         // ok

    // lv_example_imgbtn_1();

    // lv_example_keyboard_1();    // ok

    // lv_example_label_1();
    // lv_example_label_2();       // ok

    // lv_example_led_1();

    // lv_example_line_1();

    // lv_example_list_1();

    // lv_example_meter_1();
    // lv_example_meter_2();
    // lv_example_meter_3();
    // lv_example_meter_4();       // ok

    // lv_example_msgbox_1();

    // lv_example_obj_1();         // ok

    // lv_example_roller_1();
    // lv_example_roller_2();      // ok

    // lv_example_slider_1();      // ok
    // lv_example_slider_2();      // issues
    // lv_example_slider_3();      // issues

    // lv_example_spinbox_1();

    // lv_example_spinner_1();     // ok

    // lv_example_switch_1();      // ok

    // lv_example_table_1();
    // lv_example_table_2();       // ok

    // lv_example_tabview_1();

    // lv_example_textarea_1();    // ok
    // lv_example_textarea_2();
    // lv_example_textarea_3();    // ok, but not all button have functions

    // lv_example_tileview_1();    // ok

    // lv_example_win_1();         // ok

    // ----------------------------------
    // Task handler loop
    // ----------------------------------

    while (!lv_win32_quit_signal)
    {
        lv_task_handler();
        Sleep(1);
    }

    return 0;
}
