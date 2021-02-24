/*
 * PROJECT:   LVGL ported to Windows
 * FILE:      LVGL.Windows.h
 * PURPOSE:   Definition for Windows utility functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#ifndef LVGL_WINDOWS
#define LVGL_WINDOWS

#include <Windows.h>

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
    _Out_ SIZE_T* PixelBufferSize);

/**
 * @brief Retrieves the number of milliseconds that have elapsed since the
 *        system was started.
 * @return The number of milliseconds.
*/
EXTERN_C UINT64 WINAPI LvglGetTickCount();

/**
 * @brief Returns the dots per inch (dpi) value for the associated window.
 * @param WindowHandle The window you want to get information about.
 * @return The DPI for the window.
*/
EXTERN_C UINT WINAPI LvglGetDpiForWindow(
    _In_ HWND WindowHandle);

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
EXTERN_C BOOL WINAPI LvglEnableChildWindowDpiMessage(
    _In_ HWND WindowHandle);

#endif // !LVGL_WINDOWS
