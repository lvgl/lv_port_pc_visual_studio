/**
 * @file lv_windows_keypad_device.c
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

static void lv_windows_keypad_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data);

static void lv_windows_release_keypad_device_event_callback(lv_event_t* e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_indev_t* lv_windows_acquire_keypad_device(lv_display_t* display)
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

    if (!context->keypad.indev)
    {
        InitializeCriticalSection(&context->keypad.mutex);
        _lv_ll_init(
            &context->keypad.queue,
            sizeof(lv_windows_keypad_queue_item_t));
        context->keypad.utf16_high_surrogate = 0;
        context->keypad.utf16_low_surrogate = 0;

        context->keypad.indev = lv_indev_create();
        if (context->keypad.indev)
        {
            lv_indev_set_type(
                context->keypad.indev,
                LV_INDEV_TYPE_KEYPAD);
            lv_indev_set_read_cb(
                context->keypad.indev,
                lv_windows_keypad_driver_read_callback);
            lv_indev_set_display(
                context->keypad.indev,
                context->display_device_object);
            lv_indev_add_event_cb(
                context->keypad.indev,
                lv_windows_release_keypad_device_event_callback,
                LV_EVENT_DELETE,
                context->keypad.indev);
            lv_indev_set_group(
                context->keypad.indev,
                lv_group_get_default());
        }
    }

    return context->keypad.indev;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_windows_keypad_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data)
{
    lv_windows_window_context_t* context = lv_windows_get_window_context(
        lv_windows_get_indev_window_handle(indev));
    if (!context)
    {
        return;
    }

    EnterCriticalSection(&context->keypad.mutex);

    lv_windows_keypad_queue_item_t* current = (lv_windows_keypad_queue_item_t*)(
        _lv_ll_get_head(&context->keypad.queue));
    if (current)
    {
        data->key = current->key;
        data->state = current->state;

        _lv_ll_remove(&context->keypad.queue, current);
        lv_free(current);

        data->continue_reading = true;
    }

    LeaveCriticalSection(&context->keypad.mutex);
}

static void lv_windows_release_keypad_device_event_callback(lv_event_t* e)
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

    DeleteCriticalSection(&context->keypad.mutex);
    _lv_ll_clear(&context->keypad.queue);
    context->keypad.utf16_high_surrogate = 0;
    context->keypad.utf16_low_surrogate = 0;

    context->keypad.indev = NULL;
}

static void lv_windows_push_key_to_keyboard_queue(
    lv_windows_window_context_t* context,
    uint32_t key,
    lv_indev_state_t state)
{
    lv_windows_keypad_queue_item_t* current = (lv_windows_keypad_queue_item_t*)(
        _lv_ll_ins_tail(&context->keypad.queue));
    if (current)
    {
        current->key = key;
        current->state = state;
    }
}

bool lv_windows_keypad_device_window_message_handler(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            EnterCriticalSection(&context->keypad.mutex);

            bool skip_translation = false;
            uint32_t translated_key = 0;

            switch (wParam)
            {
            case VK_UP:
                translated_key = LV_KEY_UP;
                break;
            case VK_DOWN:
                translated_key = LV_KEY_DOWN;
                break;
            case VK_LEFT:
                translated_key = LV_KEY_LEFT;
                break;
            case VK_RIGHT:
                translated_key = LV_KEY_RIGHT;
                break;
            case VK_ESCAPE:
                translated_key = LV_KEY_ESC;
                break;
            case VK_DELETE:
                translated_key = LV_KEY_DEL;
                break;
            case VK_BACK:
                translated_key = LV_KEY_BACKSPACE;
                break;
            case VK_RETURN:
                translated_key = LV_KEY_ENTER;
                break;
            case VK_TAB:
            case VK_NEXT:
                translated_key = LV_KEY_NEXT;
                break;
            case VK_PRIOR:
                translated_key = LV_KEY_PREV;
                break;
            case VK_HOME:
                translated_key = LV_KEY_HOME;
                break;
            case VK_END:
                translated_key = LV_KEY_END;
                break;
            default:
                skip_translation = true;
                break;
            }

            if (!skip_translation)
            {
                lv_windows_push_key_to_keyboard_queue(
                    context,
                    translated_key,
                    ((uMsg == WM_KEYUP)
                        ? LV_INDEV_STATE_RELEASED
                        : LV_INDEV_STATE_PRESSED));
            }

            LeaveCriticalSection(&context->keypad.mutex);
        }

        break;
    }
    case WM_CHAR:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            EnterCriticalSection(&context->keypad.mutex);

            uint16_t raw_code_point = (uint16_t)(wParam);

            if (raw_code_point >= 0x20 && raw_code_point != 0x7F)
            {
                if (IS_HIGH_SURROGATE(raw_code_point))
                {
                    context->keypad.utf16_high_surrogate = raw_code_point;
                }
                else if (IS_LOW_SURROGATE(raw_code_point))
                {
                    context->keypad.utf16_low_surrogate = raw_code_point;
                }

                uint32_t code_point = raw_code_point;

                if (context->keypad.utf16_high_surrogate &&
                    context->keypad.utf16_low_surrogate)
                {
                    uint16_t high_surrogate =
                        context->keypad.utf16_high_surrogate;
                    uint16_t low_surrogate =
                        context->keypad.utf16_low_surrogate;

                    code_point = (low_surrogate & 0x03FF);
                    code_point += (((high_surrogate & 0x03FF) + 0x40) << 10);

                    context->keypad.utf16_high_surrogate = 0;
                    context->keypad.utf16_low_surrogate = 0;
                }

                uint32_t lvgl_code_point =
                    _lv_text_unicode_to_encoded(code_point);

                lv_windows_push_key_to_keyboard_queue(
                    context,
                    lvgl_code_point,
                    LV_INDEV_STATE_PRESSED);
                lv_windows_push_key_to_keyboard_queue(
                    context,
                    lvgl_code_point,
                    LV_INDEV_STATE_RELEASED);
            }

            LeaveCriticalSection(&context->keypad.mutex);
        }

        break;
    }
    case WM_IME_SETCONTEXT:
    {
        if (wParam == TRUE)
        {
            HIMC input_method_context_handle = ImmGetContext(hWnd);
            if (input_method_context_handle)
            {
                ImmAssociateContext(hWnd, input_method_context_handle);
                ImmReleaseContext(hWnd, input_method_context_handle);
            }
        }

        *plResult = DefWindowProcW(hWnd, uMsg, wParam, wParam);
        break;
    }
    case WM_IME_STARTCOMPOSITION:
    {
        HIMC input_method_context_handle = ImmGetContext(hWnd);
        if (input_method_context_handle)
        {
            lv_obj_t* textarea_object = NULL;
            lv_obj_t* focused_object = lv_group_get_focused(
                lv_group_get_default());
            if (focused_object)
            {
                const lv_obj_class_t* object_class = lv_obj_get_class(
                    focused_object);

                if (object_class == &lv_textarea_class)
                {
                    textarea_object = focused_object;
                }
                else if (object_class == &lv_keyboard_class)
                {
                    textarea_object = lv_keyboard_get_textarea(focused_object);
                }
            }

            COMPOSITIONFORM composition_form;
            composition_form.dwStyle = CFS_POINT;
            composition_form.ptCurrentPos.x = 0;
            composition_form.ptCurrentPos.y = 0;

            if (textarea_object)
            {
                lv_textarea_t* textarea = (lv_textarea_t*)(textarea_object);
                lv_obj_t* label_object = lv_textarea_get_label(textarea_object);

                composition_form.ptCurrentPos.x =
                    label_object->coords.x1 + textarea->cursor.area.x1;
                composition_form.ptCurrentPos.y =
                    label_object->coords.y1 + textarea->cursor.area.y1;
            }

            ImmSetCompositionWindow(
                input_method_context_handle,
                &composition_form);
            ImmReleaseContext(
                hWnd,
                input_method_context_handle);
        }

        *plResult = DefWindowProcW(hWnd, uMsg, wParam, wParam);
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
