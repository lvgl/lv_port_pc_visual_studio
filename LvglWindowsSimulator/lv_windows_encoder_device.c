/**
 * @file lv_windows_encoder_device.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_windows_input.h"
#ifdef LV_USE_WINDOWS

#include "lv_windows_context.h"
#include "lv_windows_interop.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void lv_windows_encoder_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data);

static void lv_windows_release_encoder_device_event_callback(lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_indev_t* lv_windows_acquire_encoder_device(lv_display_t* display)
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

    if (!context->encoder.indev)
    {
        context->encoder.state = LV_INDEV_STATE_RELEASED;
        context->encoder.enc_diff = 0;

        context->encoder.indev = lv_indev_create();
        if (context->encoder.indev)
        {
            lv_indev_set_type(
                context->encoder.indev,
                LV_INDEV_TYPE_ENCODER);
            lv_indev_set_read_cb(
                context->encoder.indev,
                lv_windows_encoder_driver_read_callback);
            lv_indev_set_display(
                context->encoder.indev,
                context->display_device_object);
            lv_indev_add_event_cb(
                context->encoder.indev,
                lv_windows_release_encoder_device_event_callback,
                LV_EVENT_DELETE,
                context->encoder.indev);
            lv_indev_set_group(
                context->encoder.indev,
                lv_group_get_default());
        }
    }

    return context->encoder.indev;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_windows_encoder_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data)
{
    lv_windows_window_context_t* context = lv_windows_get_window_context(
        lv_windows_get_indev_window_handle(indev));
    if (!context)
    {
        return;
    }

    data->state = context->encoder.state;
    data->enc_diff = context->encoder.enc_diff;
    context->encoder.enc_diff = 0;
}

static void lv_windows_release_encoder_device_event_callback(lv_event_t* e)
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

    context->encoder.state = LV_INDEV_STATE_RELEASED;
    context->encoder.enc_diff = 0;

    context->encoder.indev = NULL;
}

bool lv_windows_encoder_device_window_message_handler(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult)
{
    switch (uMsg)
    {
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            context->encoder.state = (
                uMsg == WM_MBUTTONDOWN
                ? LV_INDEV_STATE_PRESSED
                : LV_INDEV_STATE_RELEASED);
        }

        break;
    }
    case WM_MOUSEWHEEL:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            context->encoder.enc_diff =
                -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
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
