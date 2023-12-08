/**
 * @file win32drv.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "win32drv.h"

#include <windowsx.h>
#include <malloc.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#pragma comment(lib, "Imm32.lib")

/*********************
 *      DEFINES
 *********************/

#define LV_WINDOWS_ZOOM_LEVEL 100

#define LV_WINDOWS_ALLOW_DPI_OVERRIDE 0

#define WINDOW_EX_STYLE \
    WS_EX_CLIENTEDGE

#define WINDOW_STYLE \
    WS_OVERLAPPEDWINDOW //(WS_OVERLAPPEDWINDOW & ~(WS_SIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME))

#define LV_WINDOWS_ZOOM_BASE_LEVEL 100

#ifndef LV_WINDOWS_ZOOM_LEVEL
#define LV_WINDOWS_ZOOM_LEVEL LV_WINDOWS_ZOOM_BASE_LEVEL
#endif

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _WINDOW_THREAD_PARAMETER
{
    HANDLE window_mutex;
    HINSTANCE instance_handle;
    HICON icon_handle;
    int32_t hor_res;
    int32_t ver_res;
    int show_window_mode;
} WINDOW_THREAD_PARAMETER, * PWINDOW_THREAD_PARAMETER;

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**
 * @brief Creates a B8G8R8A8 frame buffer.
 * @param WindowHandle A handle to the window for the creation of the frame
 *                     buffer. If this value is NULL, the entire screen will be
 *                     referenced.
 * @param Width The width of the frame buffer.
 * @param Height The height of the frame buffer.
 * @param PixelBuffer The raw pixel buffer of the frame buffer you created.
 * @param PixelBufferSize The size of the frame buffer you created.
 * @return If the function succeeds, the return value is a handle to the device
 *         context (DC) for the frame buffer. If the function fails, the return
 *         value is NULL, and PixelBuffer parameter is NULL.
*/
static HDC lv_windows_create_frame_buffer(
    _In_opt_ HWND WindowHandle,
    _In_ LONG Width,
    _In_ LONG Height,
    _Out_ UINT32** PixelBuffer,
    _Out_ SIZE_T* PixelBufferSize);

/**
 * @brief Enables WM_DPICHANGED message for child window for the associated
 *        window.
 * @param WindowHandle The window you want to enable WM_DPICHANGED message for
 *                     child window.
 * @return If the function succeeds, the return value is non-zero. If the
 *         function fails, the return value is zero.
 * @remarks You need to use this function in Windows 10 Threshold 1 or Windows
 *          10 Threshold 2.
*/
static BOOL lv_windows_enable_child_window_dpi_message(
    _In_ HWND WindowHandle);

/**
 * @brief Registers a window as being touch-capable.
 * @param hWnd The handle of the window being registered.
 * @param ulFlags A set of bit flags that specify optional modifications.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see RegisterTouchWindow.
*/
static BOOL lv_windows_register_touch_window(
    HWND hWnd,
    ULONG ulFlags);

/**
 * @brief Retrieves detailed information about touch inputs associated with a
 *        particular touch input handle.
 * @param hTouchInput The touch input handle received in the LPARAM of a touch
 *                    message.
 * @param cInputs The number of structures in the pInputs array.
 * @param pInputs A pointer to an array of TOUCHINPUT structures to receive
 *                information about the touch points associated with the
 *                specified touch input handle.
 * @param cbSize The size, in bytes, of a single TOUCHINPUT structure.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see GetTouchInputInfo.
*/
static BOOL lv_windows_get_touch_input_info(
    HTOUCHINPUT hTouchInput,
    UINT cInputs,
    PTOUCHINPUT pInputs,
    int cbSize);

/**
 * @brief Closes a touch input handle, frees process memory associated with it,
          and invalidates the handle.
 * @param hTouchInput The touch input handle received in the LPARAM of a touch
 *                    message.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see CloseTouchInputHandle.
*/
static BOOL lv_windows_close_touch_input_handle(
    HTOUCHINPUT hTouchInput);

/**
 * @brief Returns the dots per inch (dpi) value for the associated window.
 * @param WindowHandle The window you want to get information about.
 * @return The DPI for the window.
*/
static UINT lv_windows_get_dpi_for_window(
    _In_ HWND WindowHandle);

static void lv_windows_display_driver_flush_callback(
    lv_disp_t* disp_drv,
    const lv_area_t* area,
    uint8_t* px_map);

static void lv_windows_pointer_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data);

static void lv_windows_keypad_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data);

static void lv_windows_encoder_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data);

static lv_windows_window_context_t* lv_windows_get_display_context(
    lv_disp_t* display);

static LRESULT CALLBACK lv_windows_window_message_callback(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam);

static unsigned int __stdcall lv_windows_window_thread_entrypoint(
    void* raw_parameter);

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

/**********************
 *  GLOBAL VARIABLES
 **********************/

EXTERN_C bool lv_windows_quit_signal = false;

EXTERN_C lv_indev_t* lv_windows_pointer_device_object = NULL;
EXTERN_C lv_indev_t* lv_windows_keypad_device_object = NULL;
EXTERN_C lv_indev_t* lv_windows_encoder_device_object = NULL;

/**********************
 *  STATIC VARIABLES
 **********************/

static HWND g_window_handle = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

EXTERN_C void lv_windows_add_all_input_devices_to_group(
    lv_group_t* group)
{
    if (!group)
    {
        LV_LOG_WARN(
            "The group object is NULL. Get the default group object instead.");

        group = lv_group_get_default();
        if (!group)
        {
            LV_LOG_WARN(
                "The default group object is NULL. Create a new group object "
                "and set it to default instead.");

            group = lv_group_create();
            if (group)
            {
                lv_group_set_default(group);
            }
        }
    }

    LV_ASSERT_MSG(group, "Cannot obtain an available group object.");

    lv_indev_set_group(lv_windows_pointer_device_object, group);
    lv_indev_set_group(lv_windows_keypad_device_object, group);
    lv_indev_set_group(lv_windows_encoder_device_object, group);
}

EXTERN_C lv_windows_window_context_t* lv_windows_get_window_context(
    HWND window_handle)
{
    return (lv_windows_window_context_t*)(
        GetPropW(window_handle, L"LVGL.SimulatorWindow.WindowContext"));
}

EXTERN_C bool lv_windows_init_window_class()
{
    WNDCLASSEXW window_class;
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.style = 0;
    window_class.lpfnWndProc = lv_windows_window_message_callback;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = NULL;
    window_class.hIcon = NULL;
    window_class.hCursor = LoadCursorW(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = LVGL_SIMULATOR_WINDOW_CLASS;
    window_class.hIconSm = NULL;
    return RegisterClassExW(&window_class);
}

EXTERN_C HWND lv_windows_create_display_window(
    const wchar_t* window_title,
    int32_t hor_res,
    int32_t ver_res,
    HINSTANCE instance_handle,
    HICON icon_handle,
    int show_window_mode)
{
    HWND display_window_handle = CreateWindowExW(
        WINDOW_EX_STYLE,
        LVGL_SIMULATOR_WINDOW_CLASS,
        window_title,
        WINDOW_STYLE,
        CW_USEDEFAULT,
        0,
        hor_res,
        ver_res,
        NULL,
        NULL,
        instance_handle,
        NULL);
    if (display_window_handle)
    {
        SendMessageW(
            display_window_handle,
            WM_SETICON,
            TRUE,
            (LPARAM)icon_handle);
        SendMessageW(
            display_window_handle,
            WM_SETICON,
            FALSE,
            (LPARAM)icon_handle);

        ShowWindow(display_window_handle, show_window_mode);
        UpdateWindow(display_window_handle);
    }

    return display_window_handle;
}

EXTERN_C bool lv_windows_init(
    HINSTANCE instance_handle,
    int show_window_mode,
    int32_t hor_res,
    int32_t ver_res,
    HICON icon_handle)
{
    if (!lv_windows_init_window_class())
    {
        return false;
    }

    PWINDOW_THREAD_PARAMETER parameter =
        (PWINDOW_THREAD_PARAMETER)malloc(sizeof(WINDOW_THREAD_PARAMETER));
    parameter->window_mutex = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
    parameter->instance_handle = instance_handle;
    parameter->icon_handle = icon_handle;
    parameter->hor_res = hor_res;
    parameter->ver_res = ver_res;
    parameter->show_window_mode = show_window_mode;

    _beginthreadex(
        NULL,
        0,
        lv_windows_window_thread_entrypoint,
        parameter,
        0,
        NULL);

    WaitForSingleObjectEx(parameter->window_mutex, INFINITE, FALSE);

    lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
        lv_windows_get_window_context(g_window_handle));
    if (!context)
    {
        return false;
    }

    lv_windows_pointer_device_object = context->mouse_device_object;
    lv_windows_keypad_device_object = context->keyboard_device_object;
    lv_windows_encoder_device_object = context->mousewheel_device_object;

    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static HDC lv_windows_create_frame_buffer(
    HWND WindowHandle,
    LONG Width,
    LONG Height,
    UINT32** PixelBuffer,
    SIZE_T* PixelBufferSize)
{
    HDC hFrameBufferDC = NULL;

    if (PixelBuffer && PixelBufferSize)
    {
        HDC hWindowDC = GetDC(WindowHandle);
        if (hWindowDC)
        {
            hFrameBufferDC = CreateCompatibleDC(hWindowDC);
            ReleaseDC(WindowHandle, hWindowDC);
        }

        if (hFrameBufferDC)
        {
#if (LV_COLOR_DEPTH == 32) || (LV_COLOR_DEPTH == 24)
            BITMAPINFO BitmapInfo = { 0 };
#elif (LV_COLOR_DEPTH == 16)
            typedef struct _BITMAPINFO_16BPP {
                BITMAPINFOHEADER bmiHeader;
                DWORD bmiColorMask[3];
            } BITMAPINFO_16BPP, *PBITMAPINFO_16BPP;

            BITMAPINFO_16BPP BitmapInfo = { 0 };
#else
#error [lv_windows] Unsupported LV_COLOR_DEPTH.
#endif

            BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            BitmapInfo.bmiHeader.biWidth = Width;
            BitmapInfo.bmiHeader.biHeight = -Height;
            BitmapInfo.bmiHeader.biPlanes = 1;
            BitmapInfo.bmiHeader.biBitCount = lv_color_format_get_bpp(
                LV_COLOR_FORMAT_NATIVE);
#if (LV_COLOR_DEPTH == 32) || (LV_COLOR_DEPTH == 24)
            BitmapInfo.bmiHeader.biCompression = BI_RGB;
#elif (LV_COLOR_DEPTH == 16)
            BitmapInfo.bmiHeader.biCompression = BI_BITFIELDS;
            BitmapInfo.bmiColorMask[0] = 0xF800;
            BitmapInfo.bmiColorMask[1] = 0x07E0;
            BitmapInfo.bmiColorMask[2] = 0x001F;
#else
#error [lv_windows] Unsupported LV_COLOR_DEPTH.
#endif

            HBITMAP hBitmap = CreateDIBSection(
                hFrameBufferDC,
                (PBITMAPINFO)(&BitmapInfo),
                DIB_RGB_COLORS,
                (void**)PixelBuffer,
                NULL,
                0);
            if (hBitmap)
            {
                *PixelBufferSize = Width * Height;
                *PixelBufferSize *= lv_color_format_get_size(
                    LV_COLOR_FORMAT_NATIVE);

                DeleteObject(SelectObject(hFrameBufferDC, hBitmap));
                DeleteObject(hBitmap);
            }
            else
            {
                DeleteDC(hFrameBufferDC);
                hFrameBufferDC = NULL;
            }
        }
    }

    return hFrameBufferDC;
}

static BOOL lv_windows_enable_child_window_dpi_message(
    HWND WindowHandle)
{
    // This hack is only for Windows 10 TH1/TH2 only.
    // We don't need this hack if the Per Monitor Aware V2 is existed.
    OSVERSIONINFOEXW OSVersionInfoEx = { 0 };
    OSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    OSVersionInfoEx.dwMajorVersion = 10;
    OSVersionInfoEx.dwMinorVersion = 0;
    OSVersionInfoEx.dwBuildNumber = 14393;
    if (!VerifyVersionInfoW(
        &OSVersionInfoEx,
        VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
        VerSetConditionMask(
            VerSetConditionMask(
                VerSetConditionMask(
                    0,
                    VER_MAJORVERSION,
                    VER_GREATER_EQUAL),
                VER_MINORVERSION,
                VER_GREATER_EQUAL),
            VER_BUILDNUMBER,
            VER_LESS)))
    {
        return FALSE;
    }

    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HWND, BOOL);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "EnableChildWindowDpiMessage"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(WindowHandle, TRUE);
}

static BOOL lv_windows_register_touch_window(
    HWND hWnd,
    ULONG ulFlags)
{
    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HWND, ULONG);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "RegisterTouchWindow"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hWnd, ulFlags);
}

static BOOL lv_windows_get_touch_input_info(
    HTOUCHINPUT hTouchInput,
    UINT cInputs,
    PTOUCHINPUT pInputs,
    int cbSize)
{
    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HTOUCHINPUT, UINT, PTOUCHINPUT, int);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "GetTouchInputInfo"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput, cInputs, pInputs, cbSize);
}

static BOOL lv_windows_close_touch_input_handle(
    HTOUCHINPUT hTouchInput)
{
    HMODULE ModuleHandle = GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HTOUCHINPUT);

    FunctionType pFunction = (FunctionType)(
        GetProcAddress(ModuleHandle, "CloseTouchInputHandle"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput);
}

static UINT lv_windows_get_dpi_for_window(
    _In_ HWND WindowHandle)
{
    UINT Result = (UINT)(-1);

    HMODULE ModuleHandle = LoadLibraryW(L"SHCore.dll");
    if (ModuleHandle)
    {
        typedef enum MONITOR_DPI_TYPE_PRIVATE {
            MDT_EFFECTIVE_DPI = 0,
            MDT_ANGULAR_DPI = 1,
            MDT_RAW_DPI = 2,
            MDT_DEFAULT = MDT_EFFECTIVE_DPI
        } MONITOR_DPI_TYPE_PRIVATE;

        typedef HRESULT(WINAPI* FunctionType)(
            HMONITOR, MONITOR_DPI_TYPE_PRIVATE, UINT*, UINT*);

        FunctionType pFunction = (FunctionType)(
            GetProcAddress(ModuleHandle, "GetDpiForMonitor"));
        if (pFunction)
        {
            HMONITOR MonitorHandle = MonitorFromWindow(
                WindowHandle,
                MONITOR_DEFAULTTONEAREST);

            UINT dpiX = 0;
            UINT dpiY = 0;
            if (SUCCEEDED(pFunction(
                MonitorHandle,
                MDT_EFFECTIVE_DPI,
                &dpiX,
                &dpiY)))
            {
                Result = dpiX;
            }
        }

        FreeLibrary(ModuleHandle);
    }

    if (Result == (UINT)(-1))
    {
        HDC hWindowDC = GetDC(WindowHandle);
        if (hWindowDC)
        {
            Result = GetDeviceCaps(hWindowDC, LOGPIXELSX);
            ReleaseDC(WindowHandle, hWindowDC);
        }
    }

    if (Result == (UINT)(-1))
    {
        Result = USER_DEFAULT_SCREEN_DPI;
    }

    return Result;
}

static void lv_windows_display_driver_flush_callback(
    lv_disp_t* disp_drv,
    const lv_area_t* area,
    uint8_t* px_map)
{
    HWND window_handle = (HWND)lv_display_get_driver_data(disp_drv);
    if (!window_handle)
    {
        lv_display_flush_ready(disp_drv);
        return;
    }

    lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
        lv_windows_get_window_context(window_handle));
    if (!context)
    {
        lv_display_flush_ready(disp_drv);
        return;
    }

    if (lv_display_flush_is_last(disp_drv) && !context->display_refreshing)
    {
#if (LV_COLOR_DEPTH == 32) || \
    (LV_COLOR_DEPTH == 24) || \
    (LV_COLOR_DEPTH == 16)
        UNREFERENCED_PARAMETER(px_map);
        memcpy(
            context->display_framebuffer_base,
            context->display_draw_buffer_base,
            context->display_draw_buffer_size);
#else
#error [lv_windows] Unsupported LV_COLOR_DEPTH.
#endif

        context->display_refreshing = true;

        InvalidateRect(window_handle, NULL, FALSE);
    }

    lv_display_flush_ready(disp_drv);
}

static void lv_windows_pointer_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data)
{
    lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
        lv_windows_get_display_context(lv_indev_get_disp(indev)));
    if (!context)
    {
        return;
    }

    data->state = context->pointer.state;
    data->point = context->pointer.point;
}

static void lv_windows_keypad_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data)
{
    lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
        lv_windows_get_display_context(lv_indev_get_disp(indev)));
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

static void lv_windows_encoder_driver_read_callback(
    lv_indev_t* indev,
    lv_indev_data_t* data)
{
    lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
        lv_windows_get_display_context(lv_indev_get_disp(indev)));
    if (!context)
    {
        return;
    }

    data->state = context->encoder.state;
    data->enc_diff = context->encoder.enc_diff;
    context->encoder.enc_diff = 0;
}

static lv_windows_window_context_t* lv_windows_get_display_context(
    lv_disp_t* display)
{
    if (display)
    {
        return lv_windows_get_window_context(
            (HWND)lv_display_get_driver_data(display));
    }

    return NULL;
}

static LRESULT CALLBACK lv_windows_window_message_callback(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Note: Return -1 directly because WM_DESTROY message will be sent
        // when destroy the window automatically. We free the resource when
        // processing the WM_DESTROY message of this window.

        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            malloc(sizeof(lv_windows_window_context_t)));
        if (!context)
        {
            return -1;
        }

        RECT request_content_size;
        GetWindowRect(hWnd, &request_content_size);
        context->display_device_object = lv_display_create(
            request_content_size.right - request_content_size.left,
            request_content_size.bottom - request_content_size.top);
        if (!context->display_device_object)
        {
            return -1;
        }
        lv_display_set_flush_cb(
            context->display_device_object,
            lv_windows_display_driver_flush_callback);
        lv_display_set_driver_data(
            context->display_device_object,
            hWnd);
#if !LV_WINDOWS_ALLOW_DPI_OVERRIDE
        lv_display_set_dpi(
            context->display_device_object,
            lv_windows_get_dpi_for_window(hWnd));
#endif
        context->display_refreshing = true;
        context->display_framebuffer_context_handle =
            lv_windows_create_frame_buffer(
                hWnd,
                lv_display_get_horizontal_resolution(
                    context->display_device_object),
                lv_display_get_vertical_resolution(
                    context->display_device_object),
                &context->display_framebuffer_base,
                &context->display_framebuffer_size);
        context->display_draw_buffer_size =
            lv_color_format_get_size(LV_COLOR_FORMAT_NATIVE);
        context->display_draw_buffer_size *=
            lv_display_get_horizontal_resolution(
                context->display_device_object);
        context->display_draw_buffer_size *=
            lv_display_get_vertical_resolution(
                context->display_device_object);
        context->display_draw_buffer_base =
            malloc(context->display_draw_buffer_size);
        if (!context->display_draw_buffer_base)
        {
            return -1;
        }
        lv_display_set_draw_buffers(
            context->display_device_object,
            context->display_draw_buffer_base,
            NULL,
            context->display_draw_buffer_size,
            LV_DISPLAY_RENDER_MODE_DIRECT);

        context->pointer.state = LV_INDEV_STATE_RELEASED;
        context->pointer.point.x = 0;
        context->pointer.point.y = 0;
        context->mouse_device_object = lv_indev_create();
        if (!context->mouse_device_object)
        {
            return -1;
        }
        lv_indev_set_type(
            context->mouse_device_object,
            LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(
            context->mouse_device_object,
            lv_windows_pointer_driver_read_callback);
        lv_indev_set_disp(
            context->mouse_device_object,
            context->display_device_object);

        context->encoder.state = LV_INDEV_STATE_RELEASED;
        context->encoder.enc_diff = 0;
        context->mousewheel_device_object = lv_indev_create();
        if (!context->mousewheel_device_object)
        {
            return -1;
        }
        lv_indev_set_type(
            context->mousewheel_device_object,
            LV_INDEV_TYPE_ENCODER);
        lv_indev_set_read_cb(
            context->mousewheel_device_object,
            lv_windows_encoder_driver_read_callback);
        lv_indev_set_disp(
            context->mousewheel_device_object,
            context->display_device_object);

        InitializeCriticalSection(&context->keypad.mutex);
        _lv_ll_init(
            &context->keypad.queue,
            sizeof(lv_windows_keypad_queue_item_t));
        context->keypad.utf16_high_surrogate = 0;
        context->keypad.utf16_low_surrogate = 0;
        context->keyboard_device_object = lv_indev_create();
        if (!context->keyboard_device_object)
        {
            return -1;
        }
        lv_indev_set_type(
            context->keyboard_device_object,
            LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(
            context->keyboard_device_object,
            lv_windows_keypad_driver_read_callback);
        lv_indev_set_disp(
            context->keyboard_device_object,
            context->display_device_object);
        
        if (!SetPropW(
            hWnd,
            L"LVGL.SimulatorWindow.WindowContext",
            (HANDLE)(context)))
        {
            return -1;
        }

        RECT calculated_window_size;

        calculated_window_size.left = 0;
        calculated_window_size.right = MulDiv(
            lv_display_get_horizontal_resolution(
                context->display_device_object),
            LV_WINDOWS_ZOOM_LEVEL,
            LV_WINDOWS_ZOOM_BASE_LEVEL);
        calculated_window_size.top = 0;
        calculated_window_size.bottom = MulDiv(
            lv_display_get_vertical_resolution(
                context->display_device_object),
            LV_WINDOWS_ZOOM_LEVEL,
            LV_WINDOWS_ZOOM_BASE_LEVEL);

        AdjustWindowRectEx(
            &calculated_window_size,
            WINDOW_STYLE,
            FALSE,
            WINDOW_EX_STYLE);
        OffsetRect(
            &calculated_window_size,
            -calculated_window_size.left,
            -calculated_window_size.top);

        SetWindowPos(
            hWnd,
            NULL,
            0,
            0,
            calculated_window_size.right,
            calculated_window_size.bottom,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

        lv_windows_register_touch_window(hWnd, 0);

        lv_windows_enable_child_window_dpi_message(hWnd);

        break;
    }
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

            context->pointer.point.x = MulDiv(
                GET_X_LPARAM(lParam),
                LV_WINDOWS_ZOOM_BASE_LEVEL,
                LV_WINDOWS_ZOOM_LEVEL);
            context->pointer.point.y = MulDiv(
                GET_Y_LPARAM(lParam),
                LV_WINDOWS_ZOOM_BASE_LEVEL,
                LV_WINDOWS_ZOOM_LEVEL);
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
            UINT cInputs = LOWORD(wParam);
            HTOUCHINPUT hTouchInput = (HTOUCHINPUT)(lParam);

            PTOUCHINPUT pInputs = malloc(cInputs * sizeof(TOUCHINPUT));
            if (pInputs)
            {
                if (lv_windows_get_touch_input_info(
                    hTouchInput,
                    cInputs,
                    pInputs,
                    sizeof(TOUCHINPUT)))
                {
                    for (UINT i = 0; i < cInputs; ++i)
                    {
                        POINT Point;
                        Point.x = TOUCH_COORD_TO_PIXEL(pInputs[i].x);
                        Point.y = TOUCH_COORD_TO_PIXEL(pInputs[i].y);
                        if (!ScreenToClient(hWnd, &Point))
                        {
                            continue;
                        }

                        context->pointer.point.x = MulDiv(
                            Point.x,
                            LV_WINDOWS_ZOOM_BASE_LEVEL,
                            LV_WINDOWS_ZOOM_LEVEL);
                        context->pointer.point.y = MulDiv(
                            Point.y,
                            LV_WINDOWS_ZOOM_BASE_LEVEL,
                            LV_WINDOWS_ZOOM_LEVEL);

                        DWORD MousePressedMask =
                            TOUCHEVENTF_MOVE | TOUCHEVENTF_DOWN;

                        context->pointer.state = (
                            pInputs[i].dwFlags & MousePressedMask
                            ? LV_INDEV_STATE_PRESSED
                            : LV_INDEV_STATE_RELEASED);
                    }
                }

                free(pInputs);
            }

            lv_windows_close_touch_input_handle(hTouchInput);
        }

        break;
    }
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
            HIMC hInputMethodContext = ImmGetContext(hWnd);
            if (hInputMethodContext)
            {
                ImmAssociateContext(hWnd, hInputMethodContext);
                ImmReleaseContext(hWnd, hInputMethodContext);
            }
        }

        return DefWindowProcW(hWnd, uMsg, wParam, wParam);
    }
    case WM_IME_STARTCOMPOSITION:
    {
        HIMC hInputMethodContext = ImmGetContext(hWnd);
        if (hInputMethodContext)
        {
            lv_obj_t* TextareaObject = NULL;
            lv_obj_t* FocusedObject = lv_group_get_focused(lv_group_get_default());
            if (FocusedObject)
            {
                const lv_obj_class_t* ObjectClass = lv_obj_get_class(
                    FocusedObject);

                if (ObjectClass == &lv_textarea_class)
                {
                    TextareaObject = FocusedObject;
                }
                else if (ObjectClass == &lv_keyboard_class)
                {
                    TextareaObject = lv_keyboard_get_textarea(FocusedObject);
                }
            }

            COMPOSITIONFORM CompositionForm;
            CompositionForm.dwStyle = CFS_POINT;
            CompositionForm.ptCurrentPos.x = 0;
            CompositionForm.ptCurrentPos.y = 0;

            if (TextareaObject)
            {
                lv_textarea_t* Textarea = (lv_textarea_t*)(TextareaObject);
                lv_obj_t* Label = lv_textarea_get_label(TextareaObject);

                CompositionForm.ptCurrentPos.x =
                    Label->coords.x1 + Textarea->cursor.area.x1;
                CompositionForm.ptCurrentPos.y =
                    Label->coords.y1 + Textarea->cursor.area.y1;
            }

            ImmSetCompositionWindow(hInputMethodContext, &CompositionForm);
            ImmReleaseContext(hWnd, hInputMethodContext);
        }

        return DefWindowProcW(hWnd, uMsg, wParam, wParam);
    }
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
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
                lv_windows_get_window_context(hWnd));
            if (context)
            {
                /*lv_display_set_resolution(
                    context->display_device_object,
                    LOWORD(lParam),
                    HIWORD(lParam));*/
            }
        }
        break;
    }
#if !LV_WINDOWS_ALLOW_DPI_OVERRIDE
    case WM_DPICHANGED:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            lv_display_set_dpi(context->display_device_object, HIWORD(wParam));

            LPRECT SuggestedRect = (LPRECT)lParam;

            SetWindowPos(
                hWnd,
                NULL,
                SuggestedRect->left,
                SuggestedRect->top,
                SuggestedRect->right,
                SuggestedRect->bottom,
                SWP_NOZORDER | SWP_NOACTIVATE);

            RECT ClientRect;
            GetClientRect(hWnd, &ClientRect);

            int32_t hor_res = lv_display_get_horizontal_resolution(
                context->display_device_object);
            int32_t ver_res = lv_display_get_vertical_resolution(
                context->display_device_object);

            int WindowWidth = MulDiv(
                hor_res,
                LV_WINDOWS_ZOOM_LEVEL,
                LV_WINDOWS_ZOOM_BASE_LEVEL);
            int WindowHeight = MulDiv(
                ver_res,
                LV_WINDOWS_ZOOM_LEVEL,
                LV_WINDOWS_ZOOM_BASE_LEVEL);

            SetWindowPos(
                hWnd,
                NULL,
                SuggestedRect->left,
                SuggestedRect->top,
                SuggestedRect->right + (WindowWidth - ClientRect.right),
                SuggestedRect->bottom + (WindowHeight - ClientRect.bottom),
                SWP_NOZORDER | SWP_NOACTIVATE);
        }

        break;
    }
#endif
    case WM_PAINT:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            if (context->display_framebuffer_context_handle)
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hWnd, &ps);

                SetStretchBltMode(hdc, HALFTONE);

                StretchBlt(
                    hdc,
                    ps.rcPaint.left,
                    ps.rcPaint.top,
                    ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top,
                    context->display_framebuffer_context_handle,
                    0,
                    0,
                    MulDiv(
                        ps.rcPaint.right - ps.rcPaint.left,
                        LV_WINDOWS_ZOOM_BASE_LEVEL,
                        LV_WINDOWS_ZOOM_LEVEL),
                    MulDiv(
                        ps.rcPaint.bottom - ps.rcPaint.top,
                        LV_WINDOWS_ZOOM_BASE_LEVEL,
                        LV_WINDOWS_ZOOM_LEVEL),
                    SRCCOPY);

                EndPaint(hWnd, &ps);
            }

            context->display_refreshing = false;
        }

        break;
    }
    case WM_DESTROY:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            RemovePropW(hWnd, L"LVGL.SimulatorWindow.WindowContext"));
        if (context)
        {
            lv_disp_t* display_device_object = context->display_device_object;
            context->display_device_object = NULL;
            lv_display_delete(display_device_object);
            free(context->display_draw_buffer_base);
            DeleteDC(context->display_framebuffer_context_handle);

            lv_indev_t* mouse_device_object =
                context->mouse_device_object;
            context->mouse_device_object = NULL;
            lv_indev_delete(mouse_device_object);

            lv_indev_t* mousewheel_device_object =
                context->mousewheel_device_object;
            context->mousewheel_device_object = NULL;
            lv_indev_delete(mousewheel_device_object);

            lv_indev_t* keyboard_device_object =
                context->keyboard_device_object;
            context->keyboard_device_object = NULL;
            lv_indev_delete(keyboard_device_object);
            _lv_ll_clear(&context->keypad.queue);
            DeleteCriticalSection(&context->keypad.mutex);

            free(context);
        }

        PostQuitMessage(0);

        break;
    }
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

static unsigned int __stdcall lv_windows_window_thread_entrypoint(
    void* raw_parameter)
{
    PWINDOW_THREAD_PARAMETER parameter =
        (PWINDOW_THREAD_PARAMETER)raw_parameter;

    g_window_handle = lv_windows_create_display_window(
        L"LVGL Simulator for Windows Desktop (Display 1)",
        parameter->hor_res,
        parameter->ver_res,
        parameter->instance_handle,
        parameter->icon_handle,
        parameter->show_window_mode);
    if (!g_window_handle)
    {
        return 0;
    }

    SetEvent(parameter->window_mutex);

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    lv_windows_quit_signal = true;

    return 0;
}
