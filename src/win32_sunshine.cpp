// sunshine.cpp : Defines the entry point for the console application.

#include <windows.h>
#include <windowsx.h>

#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>

#include "global_defines.h"
#include "profiling.h"
#include "win32_sunshine.h"

#include "sunshine.h"

global_variable win32_offscreen_buffer backBuffer;

global_variable bool IS_RUNNING;

#ifndef RGBA
#define RGBA(r,g,b,a)        ((COLORREF)( (((DWORD)(BYTE)(a))<<24) |     RGB(r,g,b) ))
#endif

int BUFFER_WIDTH = 32;
int BUFFER_HEIGHT = 32;

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
Win32GetRectDimension(RECT *Area)
{
    win32_window_dimension Result;
    Result.Width = Area->right - Area->left;
    Result.Height = Area->bottom - Area->top;
    return(Result);
}


static RECT
Win32GetAdjustedClientRect(HWND windowHandle)
{
    RECT ClientRect;
    GetClientRect(windowHandle, &ClientRect);
    AdjustWindowRect(&ClientRect, GetWindowStyle(windowHandle), GetMenu(windowHandle) != NULL);
    return(ClientRect);
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
    RECT ClientRect = Win32GetAdjustedClientRect(windowHandle);
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
            RECT ClientRect;
            GetClientRect(windowHandle, &ClientRect);
            win32_window_dimension clientSize = Win32GetRectDimension(&ClientRect);
            Win32DisplayBufferInWindow(&backBuffer, deviceContextHandle,
                                       clientSize.Width, clientSize.Height);
            EndPaint(windowHandle, &paintStruct);
        } break;

        case WM_SIZE:
        {
            DWORD width = LOWORD(lParam);
            DWORD height = HIWORD(lParam);
            char message[256];
            sprintf_s(message, "%i xPos %i yPos\n", width, height);
            OutputDebugStringA(message);
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
    windowClass.hCursor= LoadCursor(NULL, IDC_ARROW);
    windowClass.hIcon = LoadIcon(windowInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    windowClass.lpszClassName = "sunshineWindowClass";

    // Get processor clock speed for profiling
    QueryPerformanceFrequency(&_PCFreq);

    if (!RegisterClassA(&windowClass))
    {
        MessageBox(NULL,
                   "Call to RegisterClassEx failed!",
                   "Win32 Guided Tour",
                   NULL);
        return 1;
    }

    RECT ClientRect = {0, 0, BUFFER_WIDTH, BUFFER_HEIGHT};
    AdjustWindowRect(&ClientRect,
                     WS_OVERLAPPEDWINDOW,
                     0);

    win32_window_dimension clientInitialSize = {20 * BUFFER_WIDTH, 20 * BUFFER_HEIGHT};

    HWND windowHandle = CreateWindow(
        windowClass.lpszClassName,
        "sunshine",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        clientInitialSize.Width, clientInitialSize.Height,
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

    Win32InitBuffer(&backBuffer, BUFFER_WIDTH, BUFFER_HEIGHT);

    sunshine_offscreen_buffer Buffer = {};
    Buffer.Memory = backBuffer.Memory;
    Buffer.Width = backBuffer.Width;
    Buffer.Height = backBuffer.Height;
    Buffer.Pitch = backBuffer.Pitch;
    Buffer.BytesPerPixel = 4;

    ShowWindow(windowHandle, 3);
    UpdateWindow(windowHandle);

    while (IS_RUNNING)
    {
        MSG message;
        LARGE_INTEGER StartTime = GetCurrentClockCounter();

        while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            //float aspectRatio = float(windowWidth) / windowHeight;
            //char message1[256];
            //sprintf_s(message1, "%.04f", aspectRatio);

            if(message.message == WM_QUIT)
            {
                IS_RUNNING = false;
                PostQuitMessage(0);
            }

            if(message.message == WM_LBUTTONDOWN || message.message == WM_MOUSEMOVE)
            {
                RECT ClientRect;
                GetClientRect(windowHandle, &ClientRect);
                win32_window_dimension clientSize = Win32GetRectDimension(&ClientRect);

                int xPos = GET_X_LPARAM(message.lParam);
                int yPos = GET_Y_LPARAM(message.lParam);
                float x = float(xPos) / (float(clientSize.Width) / float(BUFFER_WIDTH));
                float y = float(yPos) / (float(clientSize.Height) / float(BUFFER_HEIGHT));

                //xPos = int(roundf(x));
                //yPos = BUFFER_HEIGHT - 1 - int(roundf(y));
                xPos = int(roundf(x - 0.5f));
                yPos = BUFFER_HEIGHT - 1 - int(roundf(y - 0.5f));

                if (message.wParam & WM_LBUTTONDOWN)
                {

                    char message[256];
                    sprintf_s(message, "%i xPos %i yPos\n", xPos, yPos);

                    //DrawRectangle(&Buffer, 0, 0, xPos, yPos, true);
                    DrawLine(&Buffer, Buffer.Width / 2, Buffer.Height / 2, xPos, yPos);
                    //DrawCircle(&Buffer, xPos, yPos, 8, true);
                    OutputDebugStringA(message);
                }
            }

            TranslateMessage(&message);
            DispatchMessageA(&message);

        }

        UpdateAndRender(&RaytracerMemory, &Buffer, Win32UpdateWindowCallback);

        RaytracerMemory.offset += 0.01f;
        if (RaytracerMemory.offset == 1.0f)
        {
            RaytracerMemory.offset = 0;
        }

        LARGE_INTEGER EndTime = GetCurrentClockCounter();
        float msElapsed = GetMilisecondsElapsed(StartTime, EndTime);

        float targetMsPerFrame = 16.6f;
        if (msElapsed < targetMsPerFrame)
        {
             Sleep(DWORD(targetMsPerFrame - msElapsed));
        }

        LARGE_INTEGER AfterSleepTime = GetCurrentClockCounter();
        msElapsed = GetMilisecondsElapsed(StartTime, AfterSleepTime);
        //PrintTime(msElapsed, "GAME LOOP");
    }

    return 0;
}

