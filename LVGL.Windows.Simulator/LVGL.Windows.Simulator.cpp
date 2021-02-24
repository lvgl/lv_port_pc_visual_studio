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
#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

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
#include "lv_examples/lv_examples.h"

#if _MSC_VER >= 1200
// Restore compilation warnings.
#pragma warning(pop)
#endif

const DWORD g_WindowExStyle = WS_EX_CLIENTEDGE;
const DWORD g_WindowStyle =
    (WS_OVERLAPPEDWINDOW & ~(WS_SIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME));

static HINSTANCE g_InstanceHandle = nullptr;
static HWND g_WindowHandle = nullptr;

static HDC g_BufferDCHandle = nullptr;
static UINT32* g_PixelBuffer = nullptr;
static SIZE_T g_PixelBufferSize = 0;

static lv_disp_t* lv_windows_disp = nullptr;

static bool volatile g_MousePressed = false;
static LPARAM volatile g_MouseValue = 0;

static bool volatile g_MouseWheelPressed = false;
static int16_t volatile g_MouseWheelValue = 0;

static bool volatile g_KeyboardPressed = false;
static WPARAM volatile g_KeyboardValue = 0;

void win_drv_flush(
    lv_disp_drv_t* disp_drv,
    const lv_area_t* area,
    lv_color_t* color_p)
{
    area;
    color_p;

    HDC hWindowDC = ::GetDC(g_WindowHandle);
    if (hWindowDC)
    {
        ::BitBlt(
            hWindowDC,
            0,
            0,
            disp_drv->hor_res,
            disp_drv->ver_res,
            g_BufferDCHandle,
            0,
            0,
            SRCCOPY);

        ::ReleaseDC(g_WindowHandle, hWindowDC);
    }

    ::lv_disp_flush_ready(disp_drv);
}

void win_drv_rounder_cb(
    lv_disp_drv_t* disp_drv,
    lv_area_t* area)
{
    area->x1 = 0;
    area->x2 = disp_drv->hor_res - 1;
    area->y1 = 0;
    area->y2 = disp_drv->ver_res - 1;
}

bool win_drv_read(
    lv_indev_drv_t* indev_drv,
    lv_indev_data_t* data)
{
    indev_drv;

    data->state = static_cast<lv_indev_state_t>(
        g_MousePressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->point.x = GET_X_LPARAM(g_MouseValue);
    data->point.y = GET_Y_LPARAM(g_MouseValue);
    return false;
}

bool win_kb_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    (void)indev_drv;      /*Unused*/

    data->state = static_cast<lv_indev_state_t>(
        g_KeyboardPressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);

    WPARAM KeyboardValue = g_KeyboardValue;

    switch (KeyboardValue)
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
    {
        int ch = ::MapVirtualKeyW((UINT)KeyboardValue, MAPVK_VK_TO_CHAR);
        if (ch)
        {
            if ((ch >= 'A' && ch <= 'Z') &&
                !(
                    (::GetKeyState(VK_SHIFT) & 0x8000) ^
                    (::GetKeyState(VK_CAPITAL) & 0x0001)))
            {
                data->key = ch + 0x20;
            }
            else
            {
                data->key = ch;
            }
        }
        break;
    }
    }

    return false;
}

bool win_mousewheel_read(lv_indev_drv_t* indev_drv, lv_indev_data_t* data)
{
    (void)indev_drv;      /*Unused*/

    data->state = static_cast<lv_indev_state_t>(
        g_MouseWheelPressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL);
    data->enc_diff = g_MouseWheelValue;
    g_MouseWheelValue = 0;

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
        g_KeyboardPressed = (uMsg == WM_KEYDOWN);
        g_KeyboardValue = wParam;
        break;
    }
    case WM_MOUSEWHEEL:
    {
        g_MouseWheelValue = -(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
        break;
    }
    case WM_DPICHANGED:
    {
        LPRECT SuggestedRect = (LPRECT)lParam;

        SetWindowPos(
            hWnd,
            nullptr,
            SuggestedRect->left,
            SuggestedRect->top,
            SuggestedRect->right,
            SuggestedRect->bottom,
            SWP_NOZORDER | SWP_NOACTIVATE);

        RECT ClientRect;
        GetClientRect(hWnd, &ClientRect);

        int WindowWidth = lv_windows_disp->driver.hor_res;
        int WindowHeight = lv_windows_disp->driver.ver_res;

        SetWindowPos(
            hWnd,
            nullptr,
            SuggestedRect->left,
            SuggestedRect->top,
            SuggestedRect->right + (WindowWidth - 1 - ClientRect.right),
            SuggestedRect->bottom + (WindowHeight - 1 - ClientRect.bottom),
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

bool g_WindowQuitSignal = false;

static void win_msg_handler(lv_task_t* param)
{
    param;

    MSG Message;
    BOOL Result = ::PeekMessageW(&Message, nullptr, 0, 0, TRUE);
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
    _In_ int nShowCmd,
    _In_ lv_coord_t hor_res,
    _In_ lv_coord_t ver_res)
{
    WNDCLASSEXW WindowClass;

    WindowClass.cbSize = sizeof(WNDCLASSEX);

    WindowClass.style = 0;
    WindowClass.lpfnWndProc = ::WndProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = hInstance;
    WindowClass.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_LVGL));
    WindowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    WindowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    WindowClass.lpszMenuName = nullptr;
    WindowClass.lpszClassName = L"lv_sim_visual_studio";
    WindowClass.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_LVGL));

    if (!::RegisterClassExW(&WindowClass))
    {
        return false;
    }

    g_InstanceHandle = hInstance;

    RECT NewWindowSize;

    NewWindowSize.left = 0;
    NewWindowSize.right = hor_res - 1;
    NewWindowSize.top = 0;
    NewWindowSize.bottom = ver_res - 1;

    AdjustWindowRectEx(
        &NewWindowSize,
        g_WindowStyle,
        FALSE,
        g_WindowExStyle);
    OffsetRect(
        &NewWindowSize,
        -NewWindowSize.left,
        -NewWindowSize.top);

    g_WindowHandle = ::CreateWindowExW(
        g_WindowExStyle,
        WindowClass.lpszClassName,
        L"LVGL Simulator for Windows Desktop",
        g_WindowStyle,
        CW_USEDEFAULT,
        0,
        NewWindowSize.right,
        NewWindowSize.bottom,
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

    HDC hNewBufferDC = ::LvglCreateFrameBuffer(
        g_WindowHandle,
        hor_res,
        ver_res,
        &g_PixelBuffer,
        &g_PixelBufferSize);

    ::DeleteDC(g_BufferDCHandle);
    g_BufferDCHandle = hNewBufferDC;

    lv_disp_buf_t* disp_buf = new lv_disp_buf_t();
    ::lv_disp_buf_init(
        disp_buf,
        g_PixelBuffer,
        nullptr,
        hor_res * ver_res);

    lv_disp_drv_t disp_drv;
    ::lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = hor_res;
    disp_drv.ver_res = ver_res;
    disp_drv.flush_cb = ::win_drv_flush;
    disp_drv.buffer = disp_buf;
    disp_drv.rounder_cb = win_drv_rounder_cb;
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

    if (!win_hal_init(hInstance, nShowCmd, LV_HOR_RES_MAX, LV_VER_RES_MAX))
    {
        return -1;
    }

    /*
     * Demos, benchmarks, and tests.
     *
     * Uncomment any one (and only one) of the functions below to run that
     * item.
     */

    lv_demo_widgets();
    //lv_demo_benchmark();
    //lv_demo_keypad_encoder();
    //lv_demo_printer();
    //lv_demo_stress();
    //lv_ex_get_started_1();
    //lv_ex_get_started_2();
    //lv_ex_get_started_3();

    //lv_ex_style_1();
    //lv_ex_style_2();
    //lv_ex_style_3();
    //lv_ex_style_4();
    //lv_ex_style_5();
    //lv_ex_style_6();
    //lv_ex_style_7();
    //lv_ex_style_8();
    //lv_ex_style_9();
    //lv_ex_style_10();
    //lv_ex_style_11();

    /*
     * There are many examples of individual widgets found under the
     * lv_examples/src/lv_ex_widgets directory.  Here are a few sample test
     * functions.  Look in that directory to find all the rest.
     */
     //lv_ex_arc_1();
     //lv_ex_cpicker_1();
     //lv_ex_gauge_1();
     //lv_ex_img_1();
     //lv_ex_tileview_1();

    while (!g_WindowQuitSignal)
    {
        ::lv_task_handler();
        ::Sleep(10);
    }

    return 0;
}
