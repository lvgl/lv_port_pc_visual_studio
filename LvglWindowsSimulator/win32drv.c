/**
 * @file win32drv.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "win32drv.h"

#include "lv_windows_context.h"

#include <malloc.h>
#include <process.h>
#include <stdbool.h>
#include <stdint.h>

#pragma comment(lib, "Imm32.lib")

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct _lv_windows_create_display_data_t
{
    const wchar_t* title;
    int32_t hor_res;
    int32_t ver_res;
    int32_t zoom_level;
    bool allow_dpi_override;
    bool simulator_mode;
    HANDLE mutex;
    lv_disp_t* display;
} lv_windows_create_display_data_t;

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
 * @brief Returns the dots per inch (dpi) value for the associated window.
 * @param WindowHandle The window you want to get information about.
 * @return The DPI for the window.
*/
static UINT lv_windows_get_dpi_for_window(
    _In_ HWND WindowHandle);

static void lv_windows_display_driver_flush_callback(
    lv_disp_t* display,
    const lv_area_t* area,
    uint8_t* px_map);

static LRESULT CALLBACK lv_windows_window_message_callback(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam);

bool lv_windows_pointer_device_window_message_handler(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult);

bool lv_windows_keypad_device_window_message_handler(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult);

bool lv_windows_encoder_device_window_message_handler(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT* plResult);

/**********************
 *  GLOBAL VARIABLES
 **********************/

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

static void lv_windows_check_display_existence_timer_callback(lv_timer_t* timer)
{
    if (!lv_display_get_next(NULL))
    {
        // Don't use lv_deinit() due to it will cause exception when parallel
        // rendering is enabled.
        exit(0);
    }
}

EXTERN_C bool lv_windows_platform_init()
{
    lv_timer_create(
        lv_windows_check_display_existence_timer_callback,
        200,
        NULL);

    // Try to ensure the default group exists.
    {
        lv_group_t* default_group = lv_group_get_default();
        if (!default_group)
        {
            default_group = lv_group_create();
            if (default_group)
            {
                lv_group_set_default(default_group);
            }
        }
    }

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
    window_class.lpszClassName = LV_WINDOWS_WINDOW_CLASS;
    window_class.hIconSm = NULL;
    return RegisterClassExW(&window_class);
}

static unsigned int __stdcall lv_windows_display_thread_entrypoint(
    void* parameter)
{
    lv_windows_create_display_data_t* data =
        (lv_windows_create_display_data_t*)(parameter);
    if (!data)
    {
        return 0;
    }

    DWORD window_style = WS_OVERLAPPEDWINDOW;
    if (data->simulator_mode)
    {
        window_style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);
    }

    HWND window_handle = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        LV_WINDOWS_WINDOW_CLASS,
        data->title,
        window_style,
        CW_USEDEFAULT,
        0,
        data->hor_res,
        data->ver_res,
        NULL,
        NULL,
        NULL,
        data);
    if (!window_handle)
    {
        return 0;
    }

    lv_windows_window_context_t* context = lv_windows_get_window_context(
        window_handle);
    if (!context)
    {
        return 0;
    }

    data->display = context->display_device_object;

    ShowWindow(window_handle, SW_SHOW);
    UpdateWindow(window_handle);

    SetEvent(data->mutex);

    data = NULL;

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return 0;
}

EXTERN_C lv_display_t* lv_windows_create_display(
    const wchar_t* title,
    int32_t hor_res,
    int32_t ver_res,
    int32_t zoom_level,
    bool allow_dpi_override,
    bool simulator_mode)
{
    lv_windows_create_display_data_t* data = NULL;
    lv_display_t* display = NULL;

    do
    {
        data = (lv_windows_create_display_data_t*)(malloc(
            sizeof(lv_windows_create_display_data_t)));
        if (!data)
        {
            break;
        }

        data->title = title;
        data->hor_res = hor_res;
        data->ver_res = ver_res;
        data->zoom_level = zoom_level;
        data->allow_dpi_override = allow_dpi_override;
        data->simulator_mode = simulator_mode;
        data->mutex = CreateEventExW(NULL, NULL, 0, EVENT_ALL_ACCESS);
        data->display = NULL;
        if (!data->mutex)
        {
            break;
        }

        _beginthreadex(
            NULL,
            0,
            lv_windows_display_thread_entrypoint,
            data,
            0,
            NULL);

        WaitForSingleObjectEx(data->mutex, INFINITE, FALSE);

    } while (false);

    if (data)
    {
        display = data->display;
        if (data->mutex)
        {
            CloseHandle(data->mutex);
        }
        free(data);
    }

    return display;
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
    // The private Per-Monitor DPI Awareness support extension is Windows 10
    // only. We don't need the private Per-Monitor DPI Awareness support
    // extension if the Per-Monitor (V2) DPI Awareness exists.
    OSVERSIONINFOEXW OSVersionInfoEx = { 0 };
    OSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    OSVersionInfoEx.dwMajorVersion = 10;
    OSVersionInfoEx.dwMinorVersion = 0;
    OSVersionInfoEx.dwBuildNumber = 14986;
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
    lv_disp_t* display,
    const lv_area_t* area,
    uint8_t* px_map)
{
    HWND window_handle = lv_windows_get_display_window_handle(display);
    if (!window_handle)
    {
        lv_display_flush_ready(display);
        return;
    }

    lv_windows_window_context_t* context = lv_windows_get_window_context(
        window_handle);
    if (!context)
    {
        lv_display_flush_ready(display);
        return;
    }

    if (lv_display_flush_is_last(display))
    {
#if (LV_COLOR_DEPTH == 32) || \
    (LV_COLOR_DEPTH == 24) || \
    (LV_COLOR_DEPTH == 16)
        UNREFERENCED_PARAMETER(px_map);
#else
#error [lv_windows] Unsupported LV_COLOR_DEPTH.
#endif

        HDC hdc = GetDC(window_handle);
        if (hdc)
        {
            SetStretchBltMode(hdc, HALFTONE);

            RECT client_rect;
            GetClientRect(window_handle, &client_rect);

            int32_t width = lv_windows_zoom_to_logical(
                client_rect.right - client_rect.left,
                context->zoom_level);
            int32_t height = lv_windows_zoom_to_logical(
                client_rect.bottom - client_rect.top,
                context->zoom_level);
            if (context->simulator_mode)
            {
                width = lv_windows_dpi_to_logical(width, context->window_dpi);
                height = lv_windows_dpi_to_logical(height, context->window_dpi);
            }

            StretchBlt(
                hdc,
                client_rect.left,
                client_rect.top,
                client_rect.right - client_rect.left,
                client_rect.bottom - client_rect.top,
                context->display_framebuffer_context_handle,
                0,
                0,
                width,
                height,
                SRCCOPY);

            ReleaseDC(window_handle, hdc);
        }
    }

    lv_display_flush_ready(display);
}

static void lv_windows_display_timer_callback(lv_timer_t* timer)
{
    lv_windows_window_context_t* context =
        (lv_windows_window_context_t*)(lv_timer_get_user_data(timer));
    if (!context)
    {
        return;
    }

    if (context->display_resolution_changed)
    {
        lv_display_set_resolution(
            context->display_device_object,
            context->requested_display_resolution.x,
            context->requested_display_resolution.y);

        int32_t hor_res = lv_display_get_horizontal_resolution(
            context->display_device_object);
        int32_t ver_res = lv_display_get_vertical_resolution(
            context->display_device_object);

        HWND window_handle = lv_windows_get_display_window_handle(
            context->display_device_object);
        if (window_handle)
        {
            if (context->display_framebuffer_context_handle)
            {
                context->display_framebuffer_base = NULL;
                context->display_framebuffer_size = 0;
                DeleteDC(context->display_framebuffer_context_handle);
                context->display_framebuffer_context_handle = NULL;
            }

            context->display_framebuffer_context_handle =
                lv_windows_create_frame_buffer(
                    window_handle,
                    hor_res,
                    ver_res,
                    &context->display_framebuffer_base,
                    &context->display_framebuffer_size);
            if (context->display_framebuffer_context_handle)
            {
                lv_display_set_buffers(
                    context->display_device_object,
                    context->display_framebuffer_base,
                    NULL,
                    context->display_framebuffer_size,
                    LV_DISPLAY_RENDER_MODE_DIRECT);
            }
        }

        context->display_resolution_changed = false;
        context->requested_display_resolution.x = 0;
        context->requested_display_resolution.y = 0;
    }
}

static LRESULT CALLBACK lv_windows_window_message_callback(
    HWND hWnd,
    UINT uMsg,
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

        lv_windows_create_display_data_t* data =
            (lv_windows_create_display_data_t*)(
                ((LPCREATESTRUCTW)(lParam))->lpCreateParams);
        if (!data)
        {
            return -1;
        }

        lv_windows_window_context_t* context =
            (lv_windows_window_context_t*)(HeapAlloc(
                GetProcessHeap(),
                HEAP_ZERO_MEMORY,
                sizeof(lv_windows_window_context_t)));
        if (!context)
        {
            return -1;
        }

        if (!SetPropW(hWnd, L"LVGL.Window.Context", (HANDLE)(context)))
        {
            return -1;
        }

        context->window_dpi = lv_windows_get_dpi_for_window(hWnd);
        context->zoom_level = data->zoom_level;
        context->allow_dpi_override = data->allow_dpi_override;
        context->simulator_mode = data->simulator_mode;

        context->display_timer_object = lv_timer_create(
            lv_windows_display_timer_callback,
            LV_DEF_REFR_PERIOD,
            context);

        context->display_resolution_changed = false;
        context->requested_display_resolution.x = 0;
        context->requested_display_resolution.y = 0;

        context->display_device_object = lv_display_create(0, 0);
        if (!context->display_device_object)
        {
            return -1;
        }
        RECT request_content_size;
        GetWindowRect(hWnd, &request_content_size);
        lv_display_set_resolution(
            context->display_device_object,
            request_content_size.right - request_content_size.left,
            request_content_size.bottom - request_content_size.top);
        lv_display_set_flush_cb(
            context->display_device_object,
            lv_windows_display_driver_flush_callback);
        lv_display_set_driver_data(
            context->display_device_object,
            hWnd);
        if (!context->allow_dpi_override)
        {
            lv_display_set_dpi(
                context->display_device_object,
                context->window_dpi);
        }

        if (context->simulator_mode)
        {
            context->display_resolution_changed = true;
            context->requested_display_resolution.x =
                lv_display_get_horizontal_resolution(
                    context->display_device_object);
            context->requested_display_resolution.y =
                lv_display_get_vertical_resolution(
                    context->display_device_object);
        }

        lv_windows_register_touch_window(hWnd, 0);

        lv_windows_enable_child_window_dpi_message(hWnd);

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
                if (!context->simulator_mode)
                {
                    context->display_resolution_changed = true;
                    context->requested_display_resolution.x = LOWORD(lParam);
                    context->requested_display_resolution.y = HIWORD(lParam);
                }
                else
                {
                    int32_t window_width = lv_windows_dpi_to_physical(
                        lv_windows_zoom_to_physical(
                            lv_display_get_horizontal_resolution(
                                context->display_device_object),
                            context->zoom_level),
                        context->window_dpi);
                    int32_t window_height = lv_windows_dpi_to_physical(
                        lv_windows_zoom_to_physical(
                            lv_display_get_vertical_resolution(
                                context->display_device_object),
                            context->zoom_level),
                        context->window_dpi);

                    RECT window_rect;
                    GetWindowRect(hWnd, &window_rect);

                    RECT client_rect;
                    GetClientRect(hWnd, &client_rect);

                    int32_t original_window_width =
                        window_rect.right - window_rect.left;
                    int32_t original_window_height =
                        window_rect.bottom - window_rect.top;

                    int32_t original_client_width =
                        client_rect.right - client_rect.left;
                    int32_t original_client_height =
                        client_rect.bottom - client_rect.top;

                    int32_t reserved_width =
                        original_window_width - original_client_width;
                    int32_t reserved_height =
                        original_window_height - original_client_height;

                    SetWindowPos(
                        hWnd,
                        NULL,
                        0,
                        0,
                        reserved_width + window_width,
                        reserved_height + window_height,
                        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
                }
            }
        }
        break;
    }
    case WM_DPICHANGED:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            context->window_dpi = HIWORD(wParam);

            if (!context->allow_dpi_override)
            {
                lv_display_set_dpi(
                    context->display_device_object,
                    context->window_dpi);
            }

            LPRECT suggested_rect = (LPRECT)lParam;

            SetWindowPos(
                hWnd,
                NULL,
                suggested_rect->left,
                suggested_rect->top,
                suggested_rect->right,
                suggested_rect->bottom,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }

        break;
    }
    case WM_ERASEBKGND:
    {
        return TRUE;
    }
    case WM_DESTROY:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            RemovePropW(hWnd, L"LVGL.Window.Context"));
        if (context)
        {
            lv_disp_t* display_device_object = context->display_device_object;
            context->display_device_object = NULL;
            lv_display_delete(display_device_object);
            DeleteDC(context->display_framebuffer_context_handle);

            lv_timer_delete(context->display_timer_object);

            HeapFree(GetProcessHeap(), 0, context);
        }

        PostQuitMessage(0);

        break;
    }
    default:
    {
        lv_windows_window_context_t* context = (lv_windows_window_context_t*)(
            lv_windows_get_window_context(hWnd));
        if (context)
        {
            LRESULT lResult = 0;
            if (context->pointer.indev &&
                lv_windows_pointer_device_window_message_handler(
                hWnd,
                uMsg,
                wParam,
                lParam,
                &lResult))
            {
                return lResult;
            }
            else if (context->keypad.indev &&
                lv_windows_keypad_device_window_message_handler(
                hWnd,
                uMsg,
                wParam,
                lParam,
                &lResult))
            {
                return lResult;
            }
            else if (context->encoder.indev &&
                lv_windows_encoder_device_window_message_handler(
                hWnd,
                uMsg,
                wParam,
                lParam,
                &lResult))
            {
                return lResult;
            }
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    }

    return 0;
}
