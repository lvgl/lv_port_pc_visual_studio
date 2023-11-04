/*
 * PROJECT:   LVGL Windows Desktop Application Demo
 * FILE:      LvglWindowsDesktopApplication.cpp
 * PURPOSE:   Implementation for LVGL Windows Desktop Application Demo
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include <Windows.h>
#include <windowsx.h>

#pragma comment(lib, "Imm32.lib")

#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#if _MSC_VER >= 1200
 // Disable compilation warnings.
#pragma warning(push)
// nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
// 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4244)
// operator 'operator-name': deprecated between enumerations of different types
#pragma warning(disable:5054)
#endif

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

/**
 * @brief Creates a B8G8R8A8 frame buffer.
 * @param WindowHandle A handle to the window for the creation of the frame
 *                     buffer. If this value is nullptr, the entire screen will
 *                     be referenced.
 * @param Width The width of the frame buffer.
 * @param Height The height of the frame buffer.
 * @param PixelBuffer The raw pixel buffer of the frame buffer you created.
 * @param PixelBufferSize The size of the frame buffer you created.
 * @return If the function succeeds, the return value is a handle to the device
 *         context (DC) for the frame buffer. If the function fails, the return
 *         value is nullptr, and PixelBuffer parameter is nullptr.
*/
EXTERN_C HDC WINAPI LvglCreateFrameBuffer(
    _In_opt_ HWND WindowHandle,
    _In_ LONG Width,
    _In_ LONG Height,
    _Out_ UINT32** PixelBuffer,
    _Out_ SIZE_T* PixelBufferSize)
{
    HDC hFrameBufferDC = nullptr;

    if (PixelBuffer && PixelBufferSize)
    {
        HDC hWindowDC = ::GetDC(WindowHandle);
        if (hWindowDC)
        {
            hFrameBufferDC = ::CreateCompatibleDC(hWindowDC);
            ::ReleaseDC(WindowHandle, hWindowDC);
        }

        if (hFrameBufferDC)
        {
            BITMAPINFO BitmapInfo = { 0 };
            BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            BitmapInfo.bmiHeader.biWidth = Width;
            BitmapInfo.bmiHeader.biHeight = -Height;
            BitmapInfo.bmiHeader.biPlanes = 1;
            BitmapInfo.bmiHeader.biBitCount = 32;
            BitmapInfo.bmiHeader.biCompression = BI_RGB;

            HBITMAP hBitmap = ::CreateDIBSection(
                hFrameBufferDC,
                &BitmapInfo,
                DIB_RGB_COLORS,
                reinterpret_cast<void**>(PixelBuffer),
                nullptr,
                0);
            if (hBitmap)
            {
                *PixelBufferSize = Width * Height * sizeof(UINT32);
                ::DeleteObject(::SelectObject(hFrameBufferDC, hBitmap));
                ::DeleteObject(hBitmap);
            }
            else
            {
                ::DeleteDC(hFrameBufferDC);
                hFrameBufferDC = nullptr;
            }
        }
    }

    return hFrameBufferDC;
}

/**
 * @brief Returns the dots per inch (dpi) value for the associated window.
 * @param WindowHandle The window you want to get information about.
 * @return The DPI for the window.
*/
EXTERN_C UINT WINAPI LvglGetDpiForWindow(
    _In_ HWND WindowHandle)
{
    UINT Result = static_cast<UINT>(-1);

    HMODULE ModuleHandle = ::LoadLibraryW(L"SHCore.dll");
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

        FunctionType pFunction = reinterpret_cast<FunctionType>(
            ::GetProcAddress(ModuleHandle, "GetDpiForMonitor"));
        if (pFunction)
        {
            HMONITOR MonitorHandle = ::MonitorFromWindow(
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

        ::FreeLibrary(ModuleHandle);
    }

    if (Result == static_cast<UINT>(-1))
    {
        HDC hWindowDC = ::GetDC(WindowHandle);
        if (hWindowDC)
        {
            Result = ::GetDeviceCaps(hWindowDC, LOGPIXELSX);
            ::ReleaseDC(WindowHandle, hWindowDC);
        }
    }

    if (Result == static_cast<UINT>(-1))
    {
        Result = USER_DEFAULT_SCREEN_DPI;
    }

    return Result;
}

EXTERN_C BOOL WINAPI LvglEnableChildWindowDpiMessage(
    HWND WindowHandle)
{
    // This hack is only for Windows 10 TH1/TH2 only.
    // We don't need this hack if the Per Monitor Aware V2 is existed.
    OSVERSIONINFOEXW OSVersionInfoEx = { 0 };
    OSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    OSVersionInfoEx.dwMajorVersion = 10;
    OSVersionInfoEx.dwMinorVersion = 0;
    OSVersionInfoEx.dwBuildNumber = 14393;
    if (!::VerifyVersionInfoW(
        &OSVersionInfoEx,
        VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER,
        ::VerSetConditionMask(
            ::VerSetConditionMask(
                ::VerSetConditionMask(
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

    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    typedef BOOL(WINAPI* FunctionType)(HWND, BOOL);

    FunctionType pFunction = reinterpret_cast<FunctionType>(
        ::GetProcAddress(ModuleHandle, "EnableChildWindowDpiMessage"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(WindowHandle, TRUE);
}

/**
 * @brief Registers a window as being touch-capable.
 * @param hWnd The handle of the window being registered.
 * @param ulFlags A set of bit flags that specify optional modifications.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see RegisterTouchWindow.
*/
EXTERN_C BOOL WINAPI LvglRegisterTouchWindow(
    _In_ HWND hWnd,
    _In_ ULONG ulFlags)
{
    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    decltype(::RegisterTouchWindow)* pFunction =
        reinterpret_cast<decltype(::RegisterTouchWindow)*>(
            ::GetProcAddress(ModuleHandle, "RegisterTouchWindow"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hWnd, ulFlags);
}

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
EXTERN_C BOOL WINAPI LvglGetTouchInputInfo(
    _In_ HTOUCHINPUT hTouchInput,
    _In_ UINT cInputs,
    _Out_ PTOUCHINPUT pInputs,
    _In_ int cbSize)
{
    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    decltype(::GetTouchInputInfo)* pFunction =
        reinterpret_cast<decltype(::GetTouchInputInfo)*>(
            ::GetProcAddress(ModuleHandle, "GetTouchInputInfo"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput, cInputs, pInputs, cbSize);
}

/**
 * @brief Closes a touch input handle, frees process memory associated with it,
          and invalidates the handle.
 * @param hTouchInput The touch input handle received in the LPARAM of a touch
 *                    message.
 * @return If the function succeeds, the return value is nonzero. If the
 *         function fails, the return value is zero.
 * @remark For more information, see CloseTouchInputHandle.
*/
EXTERN_C BOOL WINAPI LvglCloseTouchInputHandle(
    _In_ HTOUCHINPUT hTouchInput)
{
    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    decltype(::CloseTouchInputHandle)* pFunction =
        reinterpret_cast<decltype(::CloseTouchInputHandle)*>(
            ::GetProcAddress(ModuleHandle, "CloseTouchInputHandle"));
    if (!pFunction)
    {
        return FALSE;
    }

    return pFunction(hTouchInput);
}




static HINSTANCE g_InstanceHandle = nullptr;
static int volatile g_WindowWidth = 0;
static int volatile g_WindowHeight = 0;
static HWND g_WindowHandle = nullptr;
static int volatile g_WindowDPI = USER_DEFAULT_SCREEN_DPI;
static HDC g_WindowDCHandle = nullptr;

static HDC g_BufferDCHandle = nullptr;
static UINT32* g_PixelBuffer = nullptr;
static SIZE_T g_PixelBufferSize = 0;

static bool volatile g_MousePressed;
static LPARAM volatile g_MouseValue = 0;

static bool volatile g_MouseWheelPressed = false;
static int16_t volatile g_MouseWheelValue = 0;

static bool volatile g_WindowQuitSignal = false;
static bool volatile g_WindowResizingSignal = false;

std::mutex g_KeyboardMutex;
std::queue<std::pair<std::uint32_t, ::lv_indev_state_t>> g_KeyQueue;
static uint16_t volatile g_Utf16HighSurrogate = 0;
static uint16_t volatile g_Utf16LowSurrogate = 0;
static lv_group_t* volatile g_DefaultGroup = nullptr;

void LvglDisplayDriverFlushCallback(
    lv_disp_t* disp_drv,
    const lv_area_t* area,
    uint8_t* px_map)
{
    UNREFERENCED_PARAMETER(px_map);

    if (::lv_disp_flush_is_last(disp_drv))
    {
        std::int32_t Width = ::lv_area_get_width(area);
        std::int32_t Height = ::lv_area_get_height(area);

        ::BitBlt(
            g_WindowDCHandle,
            area->x1,
            area->y1,
            Width,
            Height,
            g_BufferDCHandle,
            area->x1,
            area->y1,
            SRCCOPY);
    }

    ::lv_display_flush_ready(disp_drv);
}

void LvglCreateDisplayDriver(
    lv_disp_t* disp_drv,
    int hor_res,
    int ver_res)
{
    HDC hNewBufferDC = ::LvglCreateFrameBuffer(
        g_WindowHandle,
        hor_res,
        ver_res,
        &g_PixelBuffer,
        &g_PixelBufferSize);

    ::DeleteDC(g_BufferDCHandle);
    g_BufferDCHandle = hNewBufferDC;

    ::lv_display_set_dpi(
        disp_drv,
        static_cast<std::int32_t>(g_WindowDPI));
    ::lv_display_set_flush_cb(
        disp_drv,
        ::LvglDisplayDriverFlushCallback);
    ::lv_display_set_draw_buffers(
        disp_drv,
        g_PixelBuffer,
        NULL,
        sizeof(lv_color_t) * hor_res * ver_res,
        LV_DISPLAY_RENDER_MODE_DIRECT);
}

void LvglMouseDriverReadCallback(
    lv_indev_t* indev_drv,
    lv_indev_data_t* data)
{
    UNREFERENCED_PARAMETER(indev_drv);

    data->state = static_cast<lv_indev_state_t>(
        g_MousePressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED);
    data->point.x = GET_X_LPARAM(g_MouseValue);
    data->point.y = GET_Y_LPARAM(g_MouseValue);
}

void LvglKeyboardDriverReadCallback(
    lv_indev_t* indev_drv,
    lv_indev_data_t* data)
{
    UNREFERENCED_PARAMETER(indev_drv);

    std::lock_guard<std::mutex> KeyboardMutexGuard(g_KeyboardMutex);

    if (!g_KeyQueue.empty())
    {
        auto Current = g_KeyQueue.front();

        data->key = Current.first;
        data->state = Current.second;

        g_KeyQueue.pop();
    }

    if (!g_KeyQueue.empty())
    {
        data->continue_reading = true;
    }
}

void LvglMousewheelDriverReadCallback(
    lv_indev_t* indev_drv,
    lv_indev_data_t* data)
{
    UNREFERENCED_PARAMETER(indev_drv);

    data->state = static_cast<lv_indev_state_t>(
        g_MouseWheelPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED);
    data->enc_diff = g_MouseWheelValue;
    g_MouseWheelValue = 0;
}

LRESULT CALLBACK WndProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        RECT WindowRect;
        ::GetClientRect(hWnd, &WindowRect);

        g_WindowWidth = WindowRect.right - WindowRect.left;
        g_WindowHeight = WindowRect.bottom - WindowRect.top;

        g_WindowDCHandle = ::GetDC(hWnd);

        return 0;
    }
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        g_MouseValue = lParam;
        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
        {
            g_MousePressed = (uMsg == WM_LBUTTONDOWN);
        }
        else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
        {
            g_MouseWheelPressed = (uMsg == WM_MBUTTONDOWN);
        }
        return 0;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        std::lock_guard<std::mutex> KeyboardMutexGuard(g_KeyboardMutex);

        bool SkipTranslation = false;
        std::uint32_t TranslatedKey = 0;

        switch (wParam)
        {
        case VK_UP:
            TranslatedKey = LV_KEY_UP;
            break;
        case VK_DOWN:
            TranslatedKey = LV_KEY_DOWN;
            break;
        case VK_LEFT:
            TranslatedKey = LV_KEY_LEFT;
            break;
        case VK_RIGHT:
            TranslatedKey = LV_KEY_RIGHT;
            break;
        case VK_ESCAPE:
            TranslatedKey = LV_KEY_ESC;
            break;
        case VK_DELETE:
            TranslatedKey = LV_KEY_DEL;
            break;
        case VK_BACK:
            TranslatedKey = LV_KEY_BACKSPACE;
            break;
        case VK_RETURN:
            TranslatedKey = LV_KEY_ENTER;
            break;
        case VK_NEXT:
            TranslatedKey = LV_KEY_NEXT;
            break;
        case VK_PRIOR:
            TranslatedKey = LV_KEY_PREV;
            break;
        case VK_HOME:
            TranslatedKey = LV_KEY_HOME;
            break;
        case VK_END:
            TranslatedKey = LV_KEY_END;
            break;
        default:
            SkipTranslation = true;
            break;
        }

        if (!SkipTranslation)
        {
            g_KeyQueue.push(std::make_pair(
                TranslatedKey,
                static_cast<lv_indev_state_t>(
                    (uMsg == WM_KEYUP)
                    ? LV_INDEV_STATE_RELEASED
                    : LV_INDEV_STATE_PRESSED)));
        }

        break;
    }
    case WM_CHAR:
    {
        std::lock_guard<std::mutex> KeyboardMutexGuard(g_KeyboardMutex);

        uint16_t RawCodePoint = static_cast<std::uint16_t>(wParam);

        if (RawCodePoint >= 0x20 && RawCodePoint != 0x7F)
        {
            if (IS_HIGH_SURROGATE(RawCodePoint))
            {
                g_Utf16HighSurrogate = RawCodePoint;
            }
            else if (IS_LOW_SURROGATE(RawCodePoint))
            {
                g_Utf16LowSurrogate = RawCodePoint;
            }

            uint32_t CodePoint = RawCodePoint;

            if (g_Utf16HighSurrogate && g_Utf16LowSurrogate)
            {
                CodePoint = (g_Utf16LowSurrogate & 0x03FF);
                CodePoint += (((g_Utf16HighSurrogate & 0x03FF) + 0x40) << 10);

                g_Utf16HighSurrogate = 0;
                g_Utf16LowSurrogate = 0;
            }

            uint32_t LvglCodePoint = ::_lv_text_unicode_to_encoded(CodePoint);

            g_KeyQueue.push(std::make_pair(
                LvglCodePoint,
                static_cast<lv_indev_state_t>(LV_INDEV_STATE_PRESSED)));

            g_KeyQueue.push(std::make_pair(
                LvglCodePoint,
                static_cast<lv_indev_state_t>(LV_INDEV_STATE_RELEASED)));
        }

        break;
    }
    case WM_IME_SETCONTEXT:
    {
        if (wParam == TRUE)
        {
            HIMC hInputMethodContext = ::ImmGetContext(hWnd);
            if (hInputMethodContext)
            {
                ::ImmAssociateContext(hWnd, hInputMethodContext);
                ::ImmReleaseContext(hWnd, hInputMethodContext);
            }
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, wParam);
    }
    case WM_IME_STARTCOMPOSITION:
    {
        HIMC hInputMethodContext = ::ImmGetContext(hWnd);
        if (hInputMethodContext)
        {
            lv_obj_t* TextareaObject = nullptr;
            lv_obj_t* FocusedObject = ::lv_group_get_focused(g_DefaultGroup);
            if (FocusedObject)
            {
                const lv_obj_class_t* ObjectClass = ::lv_obj_get_class(
                    FocusedObject);

                if (ObjectClass == &lv_textarea_class)
                {
                    TextareaObject = FocusedObject;
                }
                else if (ObjectClass == &lv_keyboard_class)
                {
                    TextareaObject = ::lv_keyboard_get_textarea(FocusedObject);
                }
            }

            COMPOSITIONFORM CompositionForm;
            CompositionForm.dwStyle = CFS_POINT;
            CompositionForm.ptCurrentPos.x = 0;
            CompositionForm.ptCurrentPos.y = 0;

            if (TextareaObject)
            {
                lv_textarea_t* Textarea = reinterpret_cast<lv_textarea_t*>(
                    TextareaObject);
                lv_obj_t* Label = ::lv_textarea_get_label(TextareaObject);

                CompositionForm.ptCurrentPos.x =
                    Label->coords.x1 + Textarea->cursor.area.x1;
                CompositionForm.ptCurrentPos.y =
                    Label->coords.y1 + Textarea->cursor.area.y1;
            }

            ::ImmSetCompositionWindow(hInputMethodContext, &CompositionForm);
            ::ImmReleaseContext(hWnd, hInputMethodContext);
        }

        return ::DefWindowProcW(hWnd, uMsg, wParam, wParam);
    }
    case WM_MOUSEWHEEL:
    {
        g_MouseWheelValue = -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
        break;
    }
    case WM_TOUCH:
    {
        UINT cInputs = LOWORD(wParam);
        HTOUCHINPUT hTouchInput = reinterpret_cast<HTOUCHINPUT>(lParam);

        PTOUCHINPUT pInputs = new TOUCHINPUT[cInputs];
        if (pInputs)
        {
            if (::LvglGetTouchInputInfo(
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
                    if (!::ScreenToClient(hWnd, &Point))
                    {
                        continue;
                    }

                    std::uint16_t x = static_cast<std::uint16_t>(
                        Point.x & 0xffff);
                    std::uint16_t y = static_cast<std::uint16_t>(
                        Point.y & 0xffff);

                    DWORD MousePressedMask =
                        TOUCHEVENTF_MOVE | TOUCHEVENTF_DOWN;

                    g_MouseValue = (y << 16) | x;
                    g_MousePressed = (pInputs[i].dwFlags & MousePressedMask);
                }
            }

            delete[] pInputs;
        }

        ::LvglCloseTouchInputHandle(hTouchInput);

        break;
    }
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            int CurrentWindowWidth = LOWORD(lParam);
            int CurrentWindowHeight = HIWORD(lParam);
            if (CurrentWindowWidth != g_WindowWidth ||
                CurrentWindowHeight != g_WindowHeight)
            {
                g_WindowWidth = CurrentWindowWidth;
                g_WindowHeight = CurrentWindowHeight;

                g_WindowResizingSignal = true;
            }
        }
        break;
    }
    case WM_DPICHANGED:
    {
        g_WindowDPI = HIWORD(wParam);

        // Resize the window
        auto lprcNewScale = reinterpret_cast<RECT*>(lParam);

        ::SetWindowPos(
            hWnd,
            nullptr,
            lprcNewScale->left,
            lprcNewScale->top,
            lprcNewScale->right - lprcNewScale->left,
            lprcNewScale->bottom - lprcNewScale->top,
            SWP_NOZORDER | SWP_NOACTIVATE);

        break;
    }
    case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
    default:
        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

#include <LvglWindowsIconResource.h>

bool LvglWindowsInitialize(
    _In_ HINSTANCE hInstance,
    _In_ int nShowCmd)
{
    HICON IconHandle = ::LoadIconW(
        ::GetModuleHandleW(nullptr),
        MAKEINTRESOURCE(IDI_LVGL_WINDOWS));

    WNDCLASSEXW WindowClass;

    WindowClass.cbSize = sizeof(WNDCLASSEXW);

    WindowClass.style = 0;
    WindowClass.lpfnWndProc = ::WndProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = hInstance;
    WindowClass.hIcon = IconHandle;
    WindowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    WindowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    WindowClass.lpszMenuName = nullptr;
    WindowClass.lpszClassName = L"lv_port_windows";
    WindowClass.hIconSm = IconHandle;

    if (!::RegisterClassExW(&WindowClass))
    {
        return false;
    }

    g_InstanceHandle = hInstance;

    g_WindowHandle = ::CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WindowClass.lpszClassName,
        L"LVGL Windows Desktop Application Demo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        0,
        CW_USEDEFAULT,
        0,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!g_WindowHandle)
    {
        return false;
    }

    ::LvglRegisterTouchWindow(g_WindowHandle, 0);

    ::LvglEnableChildWindowDpiMessage(g_WindowHandle);
    g_WindowDPI = ::LvglGetDpiForWindow(g_WindowHandle);

    lv_disp_t* disp_drv = ::lv_display_create(
        static_cast<std::int32_t>(g_WindowWidth),
        static_cast<std::int32_t>(g_WindowHeight));
    ::LvglCreateDisplayDriver(
        disp_drv,
        g_WindowWidth,
        g_WindowHeight);

    g_DefaultGroup = ::lv_group_create();
    ::lv_group_set_default(g_DefaultGroup);

    lv_indev_t* indev_drv = ::lv_indev_create();
    ::lv_indev_set_type(
        indev_drv,
        LV_INDEV_TYPE_POINTER);
    ::lv_indev_set_read_cb(
        indev_drv,
        ::LvglMouseDriverReadCallback);
    ::lv_indev_set_group(
        indev_drv,
        g_DefaultGroup);

    lv_indev_t* kb_drv = ::lv_indev_create();
    ::lv_indev_set_type(
        kb_drv,
        LV_INDEV_TYPE_KEYPAD);
    ::lv_indev_set_read_cb(
        kb_drv,
        ::LvglKeyboardDriverReadCallback);
    ::lv_indev_set_group(
        kb_drv,
        g_DefaultGroup);

    lv_indev_t* enc_drv = ::lv_indev_create();
    ::lv_indev_set_type(
        enc_drv,
        LV_INDEV_TYPE_ENCODER);
    ::lv_indev_set_read_cb(
        enc_drv,
        ::LvglMousewheelDriverReadCallback);
    ::lv_indev_set_group(
        enc_drv,
        g_DefaultGroup);

    ::ShowWindow(g_WindowHandle, nShowCmd);
    ::UpdateWindow(g_WindowHandle);

    return true;
}

void LvglTaskSchedulerLoop()
{
    while (!g_WindowQuitSignal)
    {
        if (g_WindowResizingSignal)
        {
            lv_disp_t* CurrentDisplay = ::lv_display_get_default();
            if (CurrentDisplay)
            {
                ::LvglCreateDisplayDriver(
                    CurrentDisplay,
                    g_WindowWidth,
                    g_WindowHeight);
                ::lv_display_set_resolution(
                    CurrentDisplay,
                    static_cast<std::int32_t>(g_WindowWidth),
                    static_cast<std::int32_t>(g_WindowHeight));

                ::lv_refr_now(CurrentDisplay);
            }

            g_WindowResizingSignal = false;
        }

        ::lv_timer_handler();
        ::Sleep(1);
    }
}

int LvglWindowsLoop()
{
    MSG Message;
    while (::GetMessageW(&Message, nullptr, 0, 0))
    {
        ::TranslateMessage(&Message);
        ::DispatchMessageW(&Message);

        if (Message.message == WM_QUIT)
        {
            g_WindowQuitSignal = true;
        }
    }

    return static_cast<int>(Message.wParam);
}

#include <thread>

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    ::lv_init();

    lv_tick_set_cb([]() -> std::uint32_t
    {
        return GetTickCount();
    });

    if (!LvglWindowsInitialize(
        hInstance,
        nShowCmd))
    {
        return -1;
    }

    ::lv_demo_widgets();
    //::lv_demo_keypad_encoder();
    //::lv_demo_benchmark(LV_DEMO_BENCHMARK_MODE_RENDER_AND_DRIVER);

    std::thread(::LvglTaskSchedulerLoop).detach();

    return ::LvglWindowsLoop();
}
