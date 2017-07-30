// sunshine.cpp : Defines the entry point for the console application.

#define global_variable static

#include "global_defines.h"
#include "sunshine.h"
#include <windows.h>
#include <windowsx.h>
#include "win32_sunshine.h"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>

global_variable win32_offscreen_buffer backBuffer;

global_variable bool IS_RUNNING;

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
            IS_RUNNING = false;
            DestroyWindow(windowHandle);
        } break;

        default:
        {
            return DefWindowProc(windowHandle, message, wParam, lParam);
        }
    }
    return(result);
}


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

    int windowWidth = 500;
    int windowHeight = 500;

    if (!RegisterClassA(&windowClass))
    {
        MessageBox(NULL,
                   "Call to RegisterClassEx failed!",
                   "Win32 Guided Tour",
                   NULL);
        return 1;
    }

    HWND windowHandle = CreateWindow(
        windowClass.lpszClassName,
        "sunshine",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
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

    // NOTE(kk): winState holds the state of the application, total size,
    // allocated memory blocks etc
    win32_state winState;
    winState.TotalSize = RaytracerMemory.PermanentStorageSize;
    LPVOID BaseAddress = (LPVOID)Terabytes(2);
    winState.RaytracerMemoryBlock = VirtualAlloc(BaseAddress,
                                                 (size_t)winState.TotalSize,
                                                 MEM_RESERVE|MEM_COMMIT,
                                                 PAGE_READWRITE);

    RaytracerMemory.PermanentStorage = winState.RaytracerMemoryBlock;
    IS_RUNNING = true;

    Win32InitBuffer(&backBuffer, windowWidth, windowHeight);

    sunshine_offscreen_buffer Buffer = {};
    Buffer.Memory = backBuffer.Memory;
    Buffer.Width = backBuffer.Width;
    Buffer.Height = backBuffer.Height;
    Buffer.Pitch = backBuffer.Pitch;
    Buffer.BytesPerPixel = 4;

    ShowWindow(windowHandle, showCode);
    UpdateWindow(windowHandle);

    // Timer
    LARGE_INTEGER StartingTime, EndingTime, ElapsedMiliseconds;
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);
    //

    while (IS_RUNNING)
    {
        QueryPerformanceCounter(&StartingTime);

        MSG message;
        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            UpdateAndRender(&RaytracerMemory, &Buffer, Win32UpdateWindowCallback);

            RECT clientRect;
            GetClientRect(windowHandle, &clientRect);
            RECT windowRect;
            GetWindowRect(windowHandle, &windowRect);
            AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, 0);

            //POINT upperLeftCorner;
            //POINT lowerRightCorner;
            //GetClientRect(windowHandle, &clientRect);

            //upperLeftCorner.x = clientRect.left;
            //upperLeftCorner.y = clientRect.top;
            //lowerRightCorner.x = clientRect.right + 1;
            //lowerRightCorner.y = clientRect.bottom + 1;
            //ClientToScreen(windowHandle, &upperLeftCorner);
            //ClientToScreen(windowHandle, &lowerRightCorner);
            //SetRect(&clientRect, upperLeftCorner.x, upperLeftCorner.y,
                    //lowerRightCorner.x, lowerRightCorner.y);

            if(message.message == WM_QUIT)
            {
                IS_RUNNING = false;
                PostQuitMessage(0);
            }
            if(message.message == WM_MOUSEMOVE)
            {
                int xPos = GET_X_LPARAM(message.lParam);
                int yPos = GET_Y_LPARAM(message.lParam);
                //char message[256];
                //POINT pos;
                //pos.x = xPos;
                //pos.y = yPos;
                //ClientToScreen(windowHandle, &pos);
                //xPos = pos.x - windowRect.left;
                //yPos = pos.y - windowRect.top;
                //sprintf_s(message, "%i xPos %i yPos\n", xPos, yPos);
                ////xPos -= clientRect.left;
                ////yPos -= clientRect.top;
                //OutputDebugStringA(message);
                //DrawRectangle(&Buffer, xPos - 10, yPos - 10, xPos + 10, yPos + 10);
                DrawRectangle(&Buffer, xPos, yPos, xPos + 1, yPos + 1);
                //DrawRectangle(&Buffer, pos.x, pos.y, pos.x + 2, pos.y + 2);
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);

            UpdateAndRender(&RaytracerMemory, &Buffer, Win32UpdateWindowCallback);
        }

        RaytracerMemory.offset++;

        // Timer
        QueryPerformanceCounter(&EndingTime);
        ElapsedMiliseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
        ElapsedMiliseconds.QuadPart *= 1000;
        ElapsedMiliseconds.QuadPart /= Frequency.QuadPart;

        int FPS = min((int)(1 / (ElapsedMiliseconds.QuadPart / 1000.0)), 9999);

        char msPerFrameBuffer[256];
        sprintf_s(msPerFrameBuffer, "%i ms/f %i FPS\n",
                  ElapsedMiliseconds.QuadPart,
                  FPS);
        //OutputDebugStringA(msPerFrameBuffer);
        //
    }

    return 0;
}

