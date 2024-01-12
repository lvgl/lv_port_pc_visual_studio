/**
 * @file lv_windows_pointer_device.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_windows_pointer_device.h"
#ifdef LV_USE_WINDOWS

#include "lv_windows_context.h"
#include "lv_windows_interop.h"

#include <windowsx.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_windows_pointer_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data);

static void lv_windows_release_pointer_device_event_callback(lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_indev_t* lv_windows_acquire_pointer_device(lv_display_t* display)
{
    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (!window_handle)
    {
        return NULL;
    }

    lv_windows_window_context_t* context = lv_windows_get_window_context(
        window_handle);
    if (!context)
    {
        return NULL;
    }

    if (!context->pointer.indev)
    {
        context->pointer.state = LV_INDEV_STATE_RELEASED;
        context->pointer.point.x = 0;
        context->pointer.point.y = 0;

        context->pointer.indev = lv_indev_create();
        if (context->pointer.indev)
        {
            lv_indev_set_type(
                context->pointer.indev,
                LV_INDEV_TYPE_POINTER);
            lv_indev_set_read_cb(
                context->pointer.indev,
                lv_windows_pointer_driver_read_callback);
            lv_indev_set_display(
                context->pointer.indev,
                context->display_device_object);
            lv_indev_add_event_cb(
                context->pointer.indev,
                lv_windows_release_pointer_device_event_callback,
                LV_EVENT_DELETE,
                context->pointer.indev);
            lv_indev_set_group(
                context->pointer.indev,
                lv_group_get_default());
        }
    }

    return context->pointer.indev;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_windows_pointer_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data)
{
    lv_windows_window_context_t* context = lv_windows_get_window_context(
        lv_windows_get_indev_window_handle(indev));
    if (!context)
    {
        return;
    }

    data->state = context->pointer.state;
    data->point = context->pointer.point;
}

static void lv_windows_release_pointer_device_event_callback(lv_event_t* e)
{
    lv_indev_t* indev = (lv_indev_t*)lv_event_get_user_data(e);
    if (!indev)
    {
        return;
    }

    HWND window_handle = lv_windows_get_indev_window_handle(indev);
    if (!window_handle)
    {
        return;
    }

    lv_windows_window_context_t* context = lv_windows_get_window_context(
        window_handle);
    if (!context)
    {
        return;
    }

    context->pointer.state = LV_INDEV_STATE_RELEASED;
    context->pointer.point.x = 0;
    context->pointer.point.y = 0;

    context->pointer.indev = NULL;
}

static BOOL lv_windows_get_touch_input_info(
    HTOUCHINPUT touch_input_handle,
    UINT input_count,
    PTOUCHINPUT inputs,
    int item_size)
{
    HMODULE module_handle = GetModuleHandleW(L"user32.dll");
    if (!module_handle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* function_type)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);

    function_type function = (function_type)(
        GetProcAddress(module_handle, "GetTouchInputInfo"));
    if (!function)
    {
        return FALSE;
    }

    return function(touch_input_handle, input_count, inputs, item_size);
}

static BOOL lv_windows_close_touch_input_handle(
    HTOUCHINPUT touch_input_handle)
{
    HMODULE module_handle = GetModuleHandleW(L"user32.dll");
    if (!module_handle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* function_type)(HTOUCHINPUT);

    function_type function = (function_type)(
        GetProcAddress(module_handle, "CloseTouchInputHandle"));
    if (!function)
    {
        return FALSE;
    }

    return function(touch_input_handle);
}

bool lv_windows_pointer_device_window_message_handler(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult)
{
    switch (uMsg)
    {
    case WM_MOUSEMOVE:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            int32_t hor_res = lv_display_get_horizontal_resolution(
                context->display_device_object);
            int32_t ver_res = lv_display_get_vertical_resolution(
                context->display_device_object);

            context->pointer.point.x = lv_windows_zoom_to_logical(
                GET_X_LPARAM(lParam),
                context->zoom_level);
            context->pointer.point.y = lv_windows_zoom_to_logical(
                GET_Y_LPARAM(lParam),
                context->zoom_level);
            if (context->simulator_mode)
            {
                context->pointer.point.x = lv_windows_dpi_to_logical(
                    context->pointer.point.x,
                    context->window_dpi);
                context->pointer.point.y = lv_windows_dpi_to_logical(
                    context->pointer.point.y,
                    context->window_dpi);
            }
            if (context->pointer.point.x < 0)
            {
                context->pointer.point.x = 0;
            }
            if (context->pointer.point.x > hor_res - 1)
            {
                context->pointer.point.x = hor_res - 1;
            }
            if (context->pointer.point.y < 0)
            {
                context->pointer.point.y = 0;
            }
            if (context->pointer.point.y > ver_res - 1)
            {
                context->pointer.point.y = ver_res - 1;
            }
        }

        break;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            context->pointer.state = (
                uMsg == WM_LBUTTONDOWN
                ? LV_INDEV_STATE_PRESSED
                : LV_INDEV_STATE_RELEASED);
        }

        break;
    }
    case WM_TOUCH:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            UINT input_count = LOWORD(wParam);
            HTOUCHINPUT touch_input_handle = (HTOUCHINPUT)(lParam);

            PTOUCHINPUT inputs = malloc(input_count * sizeof(TOUCHINPUT));
            if (inputs)
            {
                if (lv_windows_get_touch_input_info(
                    touch_input_handle,
                    input_count,
                    inputs,
                    sizeof(TOUCHINPUT)))
                {
                    for (UINT i = 0; i < input_count; ++i)
                    {
                        POINT Point;
                        Point.x = TOUCH_COORD_TO_PIXEL(inputs[i].x);
                        Point.y = TOUCH_COORD_TO_PIXEL(inputs[i].y);
                        if (!ScreenToClient(hWnd, &Point))
                        {
                            continue;
                        }

                        context->pointer.point.x = lv_windows_zoom_to_logical(
                            Point.x,
                            context->zoom_level);
                        context->pointer.point.y = lv_windows_zoom_to_logical(
                            Point.y,
                            context->zoom_level);
                        if (context->simulator_mode)
                        {
                            context->pointer.point.x = lv_windows_dpi_to_logical(
                                context->pointer.point.x,
                                context->window_dpi);
                            context->pointer.point.y = lv_windows_dpi_to_logical(
                                context->pointer.point.y,
                                context->window_dpi);
                        }

                        DWORD MousePressedMask =
                            TOUCHEVENTF_MOVE | TOUCHEVENTF_DOWN;

                        context->pointer.state = (
                            inputs[i].dwFlags & MousePressedMask
                            ? LV_INDEV_STATE_PRESSED
                            : LV_INDEV_STATE_RELEASED);
                    }
                }

                free(inputs);
            }

            lv_windows_close_touch_input_handle(touch_input_handle);
        }

        break;
    }
    default:
        // Not Handled
        return false;
    }

    // Handled
    *plResult = 0;
    return true;
}

#endif // LV_USE_WINDOWS
