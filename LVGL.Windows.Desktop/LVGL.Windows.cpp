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
