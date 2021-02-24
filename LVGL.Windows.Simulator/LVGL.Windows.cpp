/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Windows.cpp
 * PURPOSE:   Implementation for Windows utility functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include "LVGL.Windows.h"

#include <ShellScalingApi.h>
#include <VersionHelpers.h>

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

EXTERN_C UINT64 WINAPI LvglGetTickCount()
{
    LARGE_INTEGER Frequency, PerformanceCount;

    if (::QueryPerformanceFrequency(&Frequency))
    {
        if (::QueryPerformanceCounter(&PerformanceCount))
        {
            return (PerformanceCount.QuadPart * 1000 / Frequency.QuadPart);
        }
    }

    return ::GetTickCount64();
}

EXTERN_C UINT WINAPI LvglGetDpiForWindow(
    _In_ HWND WindowHandle)
{
    UINT Result = static_cast<UINT>(-1);

    HMODULE ModuleHandle = ::LoadLibraryW(L"SHCore.dll");
    if (ModuleHandle)
    {
        decltype(::GetDpiForMonitor)* pGetDpiForMonitor =
            reinterpret_cast<decltype(::GetDpiForMonitor)*>(
                ::GetProcAddress(ModuleHandle, "GetDpiForMonitor"));
        if (pGetDpiForMonitor)
        {
            HMONITOR MonitorHandle = ::MonitorFromWindow(
                WindowHandle,
                MONITOR_DEFAULTTONEAREST);

            UINT dpiX = 0;
            UINT dpiY = 0;
            if (SUCCEEDED(pGetDpiForMonitor(
                MonitorHandle,
                MONITOR_DPI_TYPE::MDT_EFFECTIVE_DPI,
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
    _In_ HWND WindowHandle)
{
    // This hack is only for Windows 10 only.
    if (!::IsWindowsVersionOrGreater(10, 0, 0))
    {
        return FALSE;
    }

    // We don't need this hack if the Per Monitor Aware V2 is existed.
    OSVERSIONINFOEXW OSVersionInfoEx = { 0 };
    OSVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    OSVersionInfoEx.dwBuildNumber = 14393;
    if (::VerifyVersionInfoW(
        &OSVersionInfoEx,
        VER_BUILDNUMBER,
        ::VerSetConditionMask(0, VER_BUILDNUMBER, VER_GREATER_EQUAL)))
    {
        return FALSE;
    }

    HMODULE ModuleHandle = ::GetModuleHandleW(L"user32.dll");
    if (!ModuleHandle)
    {
        return FALSE;
    }

    BOOL WINAPI EnableChildWindowDpiMessage(
        _In_ HWND hWnd,
        _In_ BOOL bEnable);

    decltype(EnableChildWindowDpiMessage)* pEnableChildWindowDpiMessage =
        reinterpret_cast<decltype(EnableChildWindowDpiMessage)*>(
            ::GetProcAddress(ModuleHandle, "EnableChildWindowDpiMessage"));
    if (!pEnableChildWindowDpiMessage)
    {
        return FALSE;
    }

    return pEnableChildWindowDpiMessage(WindowHandle, TRUE);
}
