/*
 * PROJECT:   LVGL ported to Windows Desktop
 * FILE:      LVGL.Windows.Desktop.cpp
 * PURPOSE:   Implementation for LVGL ported to Windows Desktop
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include "LVGL.Windows.h"

#include <Windows.h>
#include <windowsx.h>

#include <cstdint>
#include <cstring>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#include "lvgl/lvgl.h"
#include "lv_examples/lv_examples.h"

static HINSTANCE g_InstanceHandle = nullptr;
static int volatile g_WindowWidth = 0;
static int volatile g_WindowHeight = 0;
static HWND g_WindowHandle = nullptr;
static int volatile g_WindowDPI = USER_DEFAULT_SCREEN_DPI;

static HDC g_BufferDCHandle = nullptr;
static UINT32* g_PixelBuffer = nullptr;
static SIZE_T g_PixelBufferSize = 0;

static lv_disp_t* lv_windows_disp;

static bool mouse_pressed;
static int mouse_x, mouse_y;

static uint32_t volatile last_key = 0;
static lv_indev_state_t volatile state = LV_INDEV_STATE_REL;
static int16_t volatile enc_diff = 0;

void win_drv_flush(
    lv_disp_drv_t* disp_drv,
    const lv_area_t* area,
    lv_color_t* color_p)
{
    if (g_WindowWidth == 0 || g_WindowHeight == 0)
    {
        ::lv_disp_flush_ready(disp_drv);
        return;
    }

    lv_coord_t WindowWidth =
        disp_drv->rotated == 0
        ? disp_drv->hor_res
        : disp_drv->ver_res;
    lv_coord_t WindowHeight =
        disp_drv->rotated == 0
        ? disp_drv->ver_res
        : disp_drv->hor_res;

    for (int y = area->y1; y <= area->y2 && y < WindowHeight; ++y)
    {
        for (int x = area->x1; x <= area->x2; ++x)
        {
            if (x < WindowWidth)
            {
                g_PixelBuffer[y * WindowWidth + x] = ::lv_color_to32(*color_p);
            }

            ++color_p;
        }
    }

    if (::lv_disp_flush_is_last(disp_drv))
    {
        ::InvalidateRect(g_WindowHandle, nullptr, FALSE);
        ::UpdateWindow(g_WindowHandle);
    }

    ::lv_disp_flush_ready(disp_drv);
}

void lv_create_display_driver(
    lv_disp_drv_t* disp_drv,
    int hor_res,
    int ver_res)
{
    ::lv_disp_drv_init(disp_drv);

    lv_disp_buf_t* disp_buf = new lv_disp_buf_t();
    ::lv_disp_buf_init(
        disp_buf,
        new lv_color_t[hor_res * ver_res],
        nullptr,
        hor_res * ver_res);

    disp_drv->hor_res = hor_res;
    disp_drv->ver_res = ver_res;
    disp_drv->flush_cb = ::win_drv_flush;
    disp_drv->buffer = disp_buf;
    disp_drv->dpi = g_WindowDPI;
}

bool win_drv_read(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    data->state = mouse_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->point.x = mouse_x;
    data->point.y = mouse_y;
    return false;
}

std::queue<std::pair<std::uint32_t, ::lv_indev_state_t>> key_queue;
std::queue<std::pair<std::uint32_t, ::lv_indev_state_t>> char_queue;
std::mutex kb_mutex;

bool win_kb_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    (void)indev_drv;      /*Unused*/

    std::lock_guard guard(kb_mutex);

    if (!char_queue.empty())
    {
        auto current = char_queue.front();

        data->key = current.first;
        data->state = current.second;

        char_queue.pop();
    }
    else if (!key_queue.empty())
    {
        auto current = key_queue.front();

        switch (current.first)
        {
        case VK_UP:
            data->key = LV_KEY_UP;
            break;
        case VK_DOWN:
            data->key = LV_KEY_DOWN;
            break;
        case VK_LEFT:
            data->key = LV_KEY_LEFT;
            break;
        case VK_RIGHT:
            data->key = LV_KEY_RIGHT;
            break;
        case VK_ESCAPE:
            data->key = LV_KEY_ESC;
            break;
        case VK_DELETE:
            data->key = LV_KEY_DEL;
            break;
        case VK_BACK:
            data->key = LV_KEY_BACKSPACE;
            break;
        case VK_RETURN:
            data->key = LV_KEY_ENTER;
            break;
        case VK_NEXT:
            data->key = LV_KEY_NEXT;
            break;
        case VK_PRIOR:
            data->key = LV_KEY_PREV;
            break;
        case VK_HOME:
            data->key = LV_KEY_HOME;
            break;
        case VK_END:
            data->key = LV_KEY_END;
            break;
        default:
            data->key = 0;
            break;
        }
        
        data->state = current.second;

        key_queue.pop();
    }

    return false;
}

bool win_mousewheel_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    (void)indev_drv;      /*Unused*/

    data->state = state;
    data->enc_diff = enc_diff;
    enc_diff = 0;

    return false;       /*No more data to read so return false*/
}

LRESULT CALLBACK WndProc(
    _In_ HWND   hWnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
        mouse_x = GET_X_LPARAM(lParam);
        mouse_y = GET_Y_LPARAM(lParam);
        if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
        {
            mouse_pressed = (uMsg == WM_LBUTTONDOWN);
        }
        return 0;
    }
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        std::lock_guard guard(kb_mutex);

        key_queue.push(
            std::make_pair(
                wParam,
                (uMsg == WM_KEYUP) ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR));

        break;
    }
    case WM_CHAR:
    {
        std::lock_guard guard(kb_mutex);

        char_queue.push(std::make_pair(wParam, LV_INDEV_STATE_PR));

        break;
    }
    case WM_MOUSEWHEEL:
    {
        enc_diff = -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
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
;
                lv_disp_buf_t* old_disp_buf = lv_windows_disp->driver.buffer;

                lv_disp_drv_t disp_drv;
                ::lv_create_display_driver(&disp_drv, g_WindowWidth, g_WindowHeight);
                ::lv_disp_drv_update(lv_windows_disp, &disp_drv);
                delete[] old_disp_buf->buf1;
                delete old_disp_buf;

                HDC hNewBufferDC = ::LvglCreateFrameBuffer(
                    g_WindowHandle,
                    g_WindowWidth,
                    g_WindowHeight,
                    &g_PixelBuffer,
                    &g_PixelBufferSize);

                ::DeleteDC(g_BufferDCHandle);
                g_BufferDCHandle = hNewBufferDC;
            }
        }
        break;
    }
    case WM_ERASEBKGND:
    {
        ::lv_refr_now(lv_windows_disp);
        return TRUE;
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
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hDC = ::BeginPaint(hWnd, &ps);

        ::BitBlt(
            hDC,
            0,
            0,
            g_WindowWidth,
            g_WindowHeight,
            g_BufferDCHandle,
            0,
            0,
            SRCCOPY);

        ::EndPaint(hWnd, &ps); 

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

bool g_WindowQuitSignal = false;

static void win_msg_handler(lv_task_t* param)
{
    param;

    MSG Message;
    BOOL Result = PeekMessageW(&Message, NULL, 0, 0, TRUE);
    if (Result != 0 && Result != -1)
    {
        ::TranslateMessage(&Message);
        ::DispatchMessageW(&Message);

        if (Message.message == WM_QUIT)
        {
            g_WindowQuitSignal = true;
        }
    }
}

bool win_hal_init(
    _In_ HINSTANCE hInstance,
    _In_ int nShowCmd)
{
    WNDCLASSEXW WindowClass;

    WindowClass.cbSize = sizeof(WNDCLASSEX);

    WindowClass.style = 0;
    WindowClass.lpfnWndProc = ::WndProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = hInstance;
    WindowClass.hIcon = nullptr;
    WindowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    WindowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    WindowClass.lpszMenuName = nullptr;
    WindowClass.lpszClassName = L"lv_port_windows";
    WindowClass.hIconSm = nullptr;

    if (!::RegisterClassExW(&WindowClass))
    {
        return false;
    }

    g_InstanceHandle = hInstance;

    g_WindowHandle = ::CreateWindowExW(
        WS_EX_CLIENTEDGE,
        WindowClass.lpszClassName,
        L"LVGL ported to Windows Desktop",
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

    ::lv_task_create(win_msg_handler, 0, LV_TASK_PRIO_HIGHEST, nullptr);

    ::LvglEnableChildWindowDpiMessage(g_WindowHandle);
    g_WindowDPI = ::LvglGetDpiForWindow(g_WindowHandle);

    lv_disp_drv_t disp_drv;
    ::lv_create_display_driver(&disp_drv, g_WindowWidth, g_WindowHeight);
    lv_windows_disp = ::lv_disp_drv_register(&disp_drv);

    lv_indev_drv_t indev_drv;
    ::lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = ::win_drv_read;
    ::lv_indev_drv_register(&indev_drv);

    lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = win_kb_read;
    ::lv_indev_drv_register(&kb_drv);

    lv_indev_drv_t enc_drv;
    lv_indev_drv_init(&enc_drv);
    enc_drv.type = LV_INDEV_TYPE_ENCODER;
    enc_drv.read_cb = win_mousewheel_read;
    ::lv_indev_drv_register(&enc_drv);

    ::ShowWindow(g_WindowHandle, nShowCmd);
    ::UpdateWindow(g_WindowHandle);
   
    return true;
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    ::lv_init();

    if (!win_hal_init(hInstance, nShowCmd))
    {
        return -1;
    }

    //::lv_demo_widgets();
    ::lv_demo_keypad_encoder();

    UINT32 PeriodTick = 10;
    UINT64 OldTick = ::LvglGetTickCount();
    while(!g_WindowQuitSignal)
    {
        ::lv_task_handler();
        UINT64 NewTick = ::LvglGetTickCount();
        UINT32 PastTick = NewTick - OldTick;
        ::lv_tick_inc(PastTick);
        int WaitTime = static_cast<int>(PeriodTick - PastTick);
        if (WaitTime > 0)
        {
            ::Sleep(WaitTime);
        }
        OldTick = NewTick;
    }

    return 0;
}
