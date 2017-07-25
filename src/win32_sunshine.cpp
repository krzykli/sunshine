// sunshine.cpp : Defines the entry point for the console application.

#define global_variable static

#include "global_defines.h"
#include "sunshine.h"
#include <windows.h>
#include "win32_sunshine.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>

global_variable win32_offscreen_buffer backBuffer;

global_variable bool isRunning;

#ifndef RGBA
#define RGBA(r,g,b,a)        ((COLORREF)( (((DWORD)(BYTE)(a))<<24) |     RGB(r,g,b) ))
#endif

static void
Win32InitBuffer(win32_offscreen_buffer *buffer, int width, int height)
{
    if(buffer->Memory)
    {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width = width;
    buffer->Height = height;

    int BytesPerPixel = 4;

    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (buffer->Width * buffer->Height) * BytesPerPixel;
    buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    buffer->Pitch = width * BytesPerPixel;
}


static win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}


static void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}


void Win32UpdateWindowCallback()
{
    HWND windowHandle = GetActiveWindow();
    RECT ClientRect;
    GetClientRect(windowHandle, &ClientRect);
    InvalidateRect(windowHandle, &ClientRect, true);
    UpdateWindow(windowHandle);
}


LRESULT CALLBACK
Win32WindowCallback(HWND windowHandle,
                    UINT message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC deviceContextHandle;
            deviceContextHandle = BeginPaint(windowHandle, &paintStruct);
            win32_window_dimension Dimension = Win32GetWindowDimension(windowHandle);
            Win32DisplayBufferInWindow(&backBuffer, deviceContextHandle,
                                       Dimension.Width, Dimension.Height);
            EndPaint(windowHandle, &paintStruct);
        } break;

        case WM_DESTROY:
        {
            isRunning = false;
            DestroyWindow(windowHandle);
        } break;

        default:
        {
            return DefWindowProc(windowHandle, message, wParam, lParam);
        }
    }
    return(result);
}


////////////////////////////////////////////////////////
int CALLBACK
WinMain(HINSTANCE windowInstance,
        HINSTANCE prevInstance,
        LPSTR     commandLine,
        int       showCode)
{
    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32WindowCallback;
    windowClass.hInstance = windowInstance;
    windowClass.hIcon = LoadIcon(windowInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    windowClass.lpszClassName = "sunshineWindowClass";

    if (!RegisterClassA(&windowClass))
    {
        MessageBox(NULL,
                   "Call to RegisterClassEx failed!",
                   "Win32 Guided Tour",
                   NULL);
        return 1;
    }

    int res[2] = {400, 400};

    HWND windowHandle = CreateWindow(
        windowClass.lpszClassName,
        "sunshine",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        // TODO(KK): check why windows won't draw this correctly without
        // enlarging the region
        int(res[0] * 1.2), int(res[1] * 1.2),
        NULL,
        NULL,
        windowInstance,
        NULL);

    if (!windowHandle)
    {
        MessageBox(NULL,
                   "Call to CreateWindow failed!",
                   "Win32 Error",
                   NULL);
        return 1;
    }

    raytracer_memory RaytracerMemory = {};
    RaytracerMemory.PermanentStorageSize = Megabytes(64);

    // winState holds the state of the application, total size, allocated
    // memory blocks etc
    win32_state winState;
    winState.TotalSize = RaytracerMemory.PermanentStorageSize;
    LPVOID BaseAddress = (LPVOID)Terabytes(2);
    winState.RaytracerMemoryBlock = VirtualAlloc(BaseAddress,
                                                 (size_t)winState.TotalSize,
                                                 MEM_RESERVE|MEM_COMMIT,
                                                 PAGE_READWRITE);

    RaytracerMemory.PermanentStorage = winState.RaytracerMemoryBlock;
    isRunning = true;

    Win32InitBuffer(&backBuffer, res[0], res[1]);

    sunshine_offscreen_buffer Buffer = {};
    Buffer.Memory = backBuffer.Memory;
    Buffer.Width = backBuffer.Width;
    Buffer.Height = backBuffer.Height;
    Buffer.Pitch = backBuffer.Pitch;
    Buffer.BytesPerPixel = 4;


    ShowWindow(windowHandle, showCode);
    UpdateWindow(windowHandle);

    while (isRunning)
    {
        MSG message;
        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if(message.message == WM_QUIT)
            {
                isRunning = false;
                PostQuitMessage(0);
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
            UpdateAndRender(&RaytracerMemory, &Buffer, Win32UpdateWindowCallback);
        }
    }
    return 0;
}

