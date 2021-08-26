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
#include "lv_demos/lv_demo.h"
#include "lv_drivers/win32drv/win32drv.h"
#include "lv_lib_freetype/lv_freetype.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

#include <stdio.h>

lv_fs_res_t lv_win32_filesystem_driver_error_from_win32(
    DWORD Error)
{
    lv_fs_res_t res;

    switch (Error)
    {
    case ERROR_SUCCESS:
        res = LV_FS_RES_OK;
        break;
    case ERROR_BAD_UNIT:
    case ERROR_NOT_READY:
    case ERROR_CRC:
    case ERROR_SEEK:
    case ERROR_NOT_DOS_DISK:
    case ERROR_WRITE_FAULT:
    case ERROR_READ_FAULT:
    case ERROR_GEN_FAILURE:
    case ERROR_WRONG_DISK:
        res = LV_FS_RES_HW_ERR;
        break;
    case ERROR_INVALID_HANDLE:
    case ERROR_INVALID_TARGET_HANDLE:
        res = LV_FS_RES_FS_ERR;
        break;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_DRIVE:
    case ERROR_NO_MORE_FILES:
    case ERROR_SECTOR_NOT_FOUND:
    case ERROR_BAD_NETPATH:
    case ERROR_BAD_NET_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_FILENAME_EXCED_RANGE:
        res = LV_FS_RES_NOT_EX;
        break;
    case ERROR_DISK_FULL:
        res = LV_FS_RES_FULL;
        break;
    case ERROR_SHARING_VIOLATION:
    case ERROR_LOCK_VIOLATION:
    case ERROR_DRIVE_LOCKED:
        res = LV_FS_RES_LOCKED;
        break;
    case ERROR_ACCESS_DENIED:
    case ERROR_CURRENT_DIRECTORY:
    case ERROR_WRITE_PROTECT:
    case ERROR_NETWORK_ACCESS_DENIED:
    case ERROR_CANNOT_MAKE:
    case ERROR_FAIL_I24:
    case ERROR_SEEK_ON_DEVICE:
    case ERROR_NOT_LOCKED:
    case ERROR_LOCK_FAILED:
        res = LV_FS_RES_DENIED;
        break;
    case ERROR_BUSY:
        res = LV_FS_RES_BUSY;
        break;
    case ERROR_TIMEOUT:
        res = LV_FS_RES_TOUT;
        break;
    case ERROR_NOT_SAME_DEVICE:
    case ERROR_DIRECT_ACCESS_HANDLE:
        res = LV_FS_RES_NOT_IMP;
        break;
    case ERROR_TOO_MANY_OPEN_FILES:
    case ERROR_ARENA_TRASHED:
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_INVALID_BLOCK:
    case ERROR_OUT_OF_PAPER:
    case ERROR_SHARING_BUFFER_EXCEEDED:
    case ERROR_NOT_ENOUGH_QUOTA:
        res = LV_FS_RES_OUT_OF_MEM;
        break;
    case ERROR_INVALID_FUNCTION:
    case ERROR_INVALID_ACCESS:
    case ERROR_INVALID_DATA:
    case ERROR_BAD_COMMAND:
    case ERROR_BAD_LENGTH:
    case ERROR_INVALID_PARAMETER:
    case ERROR_NEGATIVE_SEEK:
        res = LV_FS_RES_INV_PARAM;
        break;
    default:
        res = LV_FS_RES_UNKNOWN;
        break;
    }

    return res;
}

static void* lv_win32_filesystem_driver_open_callback(
    lv_fs_drv_t* drv,
    const char* path,
    lv_fs_mode_t mode)
{
    UNREFERENCED_PARAMETER(drv);

    DWORD DesiredAccess = 0;

    if (mode & LV_FS_MODE_RD)
    {
        DesiredAccess |= GENERIC_READ;
    }

    if (mode & LV_FS_MODE_WR)
    {
        DesiredAccess |= GENERIC_WRITE;
    }

    char Buffer[MAX_PATH];
    sprintf(Buffer, ".\\%s", path);

    return (void*)CreateFileA(
        Buffer,
        DesiredAccess,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
}

static lv_fs_res_t lv_win32_filesystem_driver_close_callback(
    lv_fs_drv_t* drv,
    void* file_p)
{
    UNREFERENCED_PARAMETER(drv);

    return CloseHandle((HANDLE)file_p)
        ? LV_FS_RES_OK
        : lv_win32_filesystem_driver_error_from_win32(GetLastError());
}

static lv_fs_res_t lv_win32_filesystem_driver_read_callback(
    lv_fs_drv_t* drv,
    void* file_p,
    void* buf,
    uint32_t btr,
    uint32_t* br)
{
    UNREFERENCED_PARAMETER(drv);

    return ReadFile((HANDLE)file_p, buf, btr, (LPDWORD)br, NULL)
        ? LV_FS_RES_OK
        : lv_win32_filesystem_driver_error_from_win32(GetLastError());
}

static lv_fs_res_t lv_win32_filesystem_driver_write_callback(
    lv_fs_drv_t* drv,
    void* file_p,
    const void* buf,
    uint32_t btw,
    uint32_t* bw)
{
    UNREFERENCED_PARAMETER(drv);

    return WriteFile((HANDLE)file_p, buf, btw, (LPDWORD)bw, NULL)
        ? LV_FS_RES_OK
        : lv_win32_filesystem_driver_error_from_win32(GetLastError()); 
}

static lv_fs_res_t lv_win32_filesystem_driver_seek_callback(
    lv_fs_drv_t* drv,
    void* file_p,
    uint32_t pos,
    lv_fs_whence_t whence)
{
    UNREFERENCED_PARAMETER(drv);

    DWORD MoveMethod = (DWORD)-1;
    if (whence == LV_FS_SEEK_SET)
    {
        MoveMethod = FILE_BEGIN;
    }
    else if(whence == LV_FS_SEEK_CUR)
    {
        MoveMethod = FILE_CURRENT;
    }
    else if(whence == LV_FS_SEEK_END)
    {
        MoveMethod = FILE_END;
    }

    LARGE_INTEGER DistanceToMove;
    DistanceToMove.QuadPart = pos;
    return SetFilePointerEx((HANDLE)file_p, DistanceToMove, NULL, MoveMethod)
        ? LV_FS_RES_OK
        : lv_win32_filesystem_driver_error_from_win32(GetLastError());
}

static lv_fs_res_t lv_win32_filesystem_driver_tell_callback(
    lv_fs_drv_t* drv,
    void* file_p,
    uint32_t* pos_p)
{
    UNREFERENCED_PARAMETER(drv);

    if (!pos_p)
    {
        return LV_FS_RES_INV_PARAM;
    }
    *pos_p = (uint32_t)-1;

    LARGE_INTEGER FilePointer;
    FilePointer.QuadPart = 0;

    LARGE_INTEGER DistanceToMove;
    DistanceToMove.QuadPart = 0;
    if (SetFilePointerEx(
        (HANDLE)file_p,
        DistanceToMove,
        &FilePointer,
        FILE_CURRENT))
    {
        if (FilePointer.QuadPart > LONG_MAX)
        {
            return LV_FS_RES_INV_PARAM;
        }
        else
        {
            *pos_p = FilePointer.LowPart;
            return LV_FS_RES_OK;
        }
    }
    else
    {
        return lv_win32_filesystem_driver_error_from_win32(GetLastError());
    }
}

static char next_filename_buffer[MAX_PATH];
static lv_fs_res_t next_filename_error = LV_FS_RES_OK;

static bool is_dots_name(
    _In_ LPCSTR Name)
{
    return Name[0] == L'.' && (!Name[1] || (Name[1] == L'.' && !Name[2]));
}

static void* lv_win32_filesystem_driver_dir_open_callback(
    lv_fs_drv_t* drv,
    const char* path)
{
    UNREFERENCED_PARAMETER(drv);

    HANDLE FindHandle = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA FindData;

    char Buffer[256];
    sprintf(Buffer, ".\\%s\\*", path);

    strcpy(next_filename_buffer, "");

    FindHandle = FindFirstFileA(Buffer, &FindData);
    do
    {
        if (is_dots_name(FindData.cFileName))
        {
            continue;
        }
        else
        {
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                sprintf(next_filename_buffer, "/%s", FindData.cFileName);
            }
            else
            {
                sprintf(next_filename_buffer, "%s", FindData.cFileName);
            }

            break;
        }

    } while (FindNextFileA(FindHandle, &FindData));

    next_filename_error =
        lv_win32_filesystem_driver_error_from_win32(GetLastError());

    return (void*)FindHandle;
}

static lv_fs_res_t lv_win32_filesystem_driver_dir_read_callback(
    lv_fs_drv_t* drv,
    void* rddir_p,
    char* fn)
{
    UNREFERENCED_PARAMETER(drv);

    strcpy(fn, next_filename_buffer);
    lv_fs_res_t current_filename_error = next_filename_error;
    next_filename_error = LV_FS_RES_OK;

    strcpy(next_filename_buffer, "");
    WIN32_FIND_DATAA FindData;

    while (FindNextFileA((HANDLE)rddir_p, &FindData))
    {
        if (is_dots_name(FindData.cFileName))
        {
            continue;
        }
        else
        {
            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                sprintf(next_filename_buffer, "/%s", FindData.cFileName);
            }
            else
            {
                sprintf(next_filename_buffer, "%s", FindData.cFileName);
            }

            break;
        }
    }

    if (next_filename_buffer[0] == '\0')
    {
        next_filename_error =
            lv_win32_filesystem_driver_error_from_win32(GetLastError());
    }

    return current_filename_error;
}

static lv_fs_res_t lv_win32_filesystem_driver_dir_close_callback(
    lv_fs_drv_t* drv,
    void* rddir_p)
{
    UNREFERENCED_PARAMETER(drv);

    return FindClose((HANDLE)rddir_p)
        ? LV_FS_RES_OK
        : lv_win32_filesystem_driver_error_from_win32(GetLastError());
}

void lv_win32_filesystem_driver_initialize()
{
    static lv_fs_drv_t filesystem_driver;
    lv_fs_drv_init(&filesystem_driver);

    filesystem_driver.letter = '/';

    filesystem_driver.open_cb = lv_win32_filesystem_driver_open_callback;
    filesystem_driver.close_cb = lv_win32_filesystem_driver_close_callback;
    filesystem_driver.read_cb = lv_win32_filesystem_driver_read_callback;
    filesystem_driver.write_cb = lv_win32_filesystem_driver_write_callback;
    filesystem_driver.seek_cb = lv_win32_filesystem_driver_seek_callback;
    filesystem_driver.tell_cb = lv_win32_filesystem_driver_tell_callback;

    filesystem_driver.dir_open_cb = lv_win32_filesystem_driver_dir_open_callback;
    filesystem_driver.dir_read_cb = lv_win32_filesystem_driver_dir_read_callback;
    filesystem_driver.dir_close_cb = lv_win32_filesystem_driver_dir_close_callback;

    lv_fs_drv_register(&filesystem_driver);
}

int main()
{
    lv_init();

    if (!lv_win32_init(
        GetModuleHandleW(NULL),
        SW_SHOW,
        800,
        480,
        LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_LVGL))))
    {
        return -1;
    }

    lv_win32_add_all_input_devices_to_group(NULL);

    lv_win32_filesystem_driver_initialize();

    /*
     * Demos, benchmarks, and tests.
     *
     * Uncomment any one (and only one) of the functions below to run that
     * item.
     */

    // ----------------------------------
    // my application
    // ----------------------------------

    ///*Init freetype library
    // *Cache max 64 faces and 1 size*/
    //lv_freetype_init(64, 1, 0);

    ///*Create a font*/
    //static lv_ft_info_t info;
    //info.name = "./lv_lib_freetype/arial.ttf";
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
    // Demos from lv_examples
    // ----------------------------------

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
    }

    lv_demo_widgets();           // ok
    // lv_demo_benchmark();
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
