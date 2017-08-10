#ifndef RAYKH
#define RAYKH

#include <math.h>

struct raytracer_memory
{
    bool IsInitialized;
    float offset;
    uint64 PermanentStorageSize;
    void *PermanentStorage;
};


struct renderStats
{
    float renderTime;
    int aaSamples;
    int resolution[2];
};


struct sunshine_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};


struct Color3f
{
     float r;
     float g;
     float b;
};


Color3f
lerpColor3f(Color3f &a, Color3f &b, float t)
{
    Color3f result;
    result.r = a.r + t * (b.r - a.r);
    result.g = a.g + t * (b.g - a.g);
    result.b = a.b + t * (b.b - a.b);
    return result;
}


uint32
Color3f_to_uint32(Color3f &color)
{
     uint8 Red = uint8(color.r * 255);
     uint8 Green = uint8(color.g * 255);
     uint8 Blue = uint8(color.b * 255);
     return ((Red << 16) | (Green << 8) | Blue);
}

Color3f
uint32_to_Colorf(uint32 color)
{
    Color3f Result;

    uint32 maskRed = 0x00FF0000;
    uint32 maskGreen = 0x0000FF00;
    uint32 maskBlue = 0x000000FF;

    unsigned char Red = unsigned char((color & maskRed) >> 16);
    unsigned char Green = unsigned char((color & maskGreen) >> 8);
    unsigned char Blue = unsigned char((color & maskBlue));

    Result.r = float(Red / 255.0);
    Result.g = float(Green / 255.0);
    Result.b = float(Blue / 255.0);

    return Result;
}

bool
IsPixelWithinFrameBufferBounds(sunshine_offscreen_buffer *Buffer, int x0, int y0)
{
    if (x0 < 0 || y0 < 0 || x0 > Buffer->Width - 1 || y0 > Buffer->Height - 1)
    {
        return false;
    }
    else
    {
         return true;
    }
}


void*
GetPixelAddress(sunshine_offscreen_buffer *Buffer, int x, int y)
{
    uint8 *row = (uint8 *)Buffer->Memory;
    int offsetFromOrigin = y * Buffer->Pitch + Buffer->BytesPerPixel * x;
    return row += offsetFromOrigin;
}

void
ColorPixel(sunshine_offscreen_buffer *Buffer, uint32 Color, int x, int y)
{
    if (IsPixelWithinFrameBufferBounds(Buffer, x, y))
    {
        uint32 *Pixel = (uint32 *)GetPixelAddress(Buffer, x, y);
        *Pixel = Color;
    }
}


void
DrawCircle(sunshine_offscreen_buffer *Buffer, int x0, int y0, int radius)
{

    LARGE_INTEGER StartTime = GetCurrentClockCounter();

    uint8 Red = 255;
    uint8 Green = 0;
    uint8 Blue = 0;
    uint32 Color = ((Red << 16) | (Green << 8) | Blue);

    int x = radius - 1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (x >= y)
    {
        ColorPixel(Buffer, Color, x0 + x, y0 + y);
        ColorPixel(Buffer, Color, x0 + y, y0 + x);
        ColorPixel(Buffer, Color, x0 - y, y0 - x);
        ColorPixel(Buffer, Color, x0 - x, y0 - y);
        ColorPixel(Buffer, Color, x0 - x, y0 + y);
        ColorPixel(Buffer, Color, x0 + x, y0 - y);
        ColorPixel(Buffer, Color, x0 - y, y0 + x);
        ColorPixel(Buffer, Color, x0 + y, y0 - x);

        if (err <= 0)
        {
            y++;
            err += dy;
            dy +=2;
        }
        if (err > 0)
        {
            x--;
            dx += 2;
            err += (-radius << 1) + dx;
        }
    }


    LARGE_INTEGER EndTime = GetCurrentClockCounter();
    char msPerFrameBuffer[512];
    sprintf_s(msPerFrameBuffer, "Ticks: %i\n", EndTime.QuadPart - StartTime.QuadPart);
    OutputDebugStringA(msPerFrameBuffer);

}

bool
IsAddressWithinFrameBufferBounds(sunshine_offscreen_buffer *Buffer, void* Pixel)
{
    if ((uint8 *)Buffer->Memory > (uint8 *)Pixel ||
        (uint8 *)GetPixelAddress(Buffer, Buffer->Width - 1, Buffer->Height - 1) < (uint8 *)Pixel)
    {
        return false;
    }
    else
    {
         return true;
    }
}



void
DrawRectangle(sunshine_offscreen_buffer *Buffer, int x1, int y1, int x2, int y2)
{
    // NOTE(kk): assume for now point A has lower coords
    //
    // Check bounds
    x2 = min(x2, Buffer->Width - 1);
    y2 = min(y2, Buffer->Height - 1);
    
    uint8 Red = 240;
    uint8 Green = 30;
    uint8 Blue = 140;

    uint8 *row = (uint8 *)GetPixelAddress(Buffer, x1, y1);

    for (int y = y1; y <= y2; ++y)
    {
        uint32 *Pixel = (uint32 *)row;
        for (int x = x1; x <= x2; ++x)
        {
            if (IsAddressWithinFrameBufferBounds(Buffer, Pixel))
            {
                *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
            }
        }
        row += Buffer->Pitch;
    }
}


void
DrawLine(sunshine_offscreen_buffer *Buffer, int x1, int y1, int x2, int y2,
        int thickness)
{
    uint8 Red = 0;
    uint8 Green = 100;
    uint8 Blue = 255;

    int dx = x2 - x1;
    int dy = y2 - y1;

    float a;
    if (dx == 0 || dy == 0)
    {
        DrawRectangle(Buffer, x1, y1, x2, y2);
    }
    else
    {
        a = float(dy) / dx;

        for (int t = thickness; t > 0; t--)
        {
            for (int x = x1; x <= x2; ++x)
            {
                float y = a * x + y1 + t;
                uint32 *Pixel = (uint32 *)GetPixelAddress(Buffer, int(x), int(y));
                *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
            }
        }
    }
}

void
UpdateAndRender(raytracer_memory *Memory,
                sunshine_offscreen_buffer *Buffer,
                void (*WindowUpdateCallback)(void))
{
    // Get application state from Memory
    if (!Memory->IsInitialized)
    {
        Memory->offset = 0.0f;
        Memory->IsInitialized = 1;
    }

    float offset = Memory->offset;
    float cycleOffset = float((sin(5 * offset * 3.14f) + 1.0f)) / 2.0f;
    //float cycleOffset = offset;

    Color3f corner1 = {1.0f - cycleOffset, 0.0f + cycleOffset, 0.0f + cycleOffset};
    Color3f corner2 = {1.0f, 1.0f - cycleOffset, 0.0f};
    Color3f corner3 = {0.0f + cycleOffset, 0.0f + cycleOffset, 1.0f - cycleOffset};
    Color3f corner4 = {0.0f, 1.0f - cycleOffset, 1.0f};

    uint8 *row = (uint8 *)Buffer->Memory;

    LARGE_INTEGER StartTime = GetCurrentClockCounter();
    for (int y=0; y < Buffer->Height; ++y)
    {
        uint32 *Pixel = (uint32 *)row;

        for (int x=0; x < Buffer->Width; ++x)
        {
            //float u = float(x) / (Buffer->Width - 1);
            //float v = float(y) / (Buffer->Height - 1);


            //Color3f colorUpperX = lerpColor3f(corner1, corner2, u);
            ////Color3f colorLowerX = lerpColor3f(corner3, corner4, u);
            ////Color3f color = lerpColor3f(colorUpperX, colorLowerX, v);

            //*Pixel++ = Color3f_to_uint32(colorUpperX);
        }


        row += Buffer->Pitch;
    }
    //DrawRectangle(Buffer, 0, 0, 20, 20);
    //DrawRectangle(Buffer, Buffer->Width - 20, Buffer->Height - 20,
                  //Buffer->Width - 1, Buffer->Height - 1);
    //DrawLine(Buffer, 0, 0, Buffer->Width - 1, Buffer->Height - 1, 5);
    //DrawLine(Buffer, 0, Buffer->Height - 1, Buffer->Width - 1, 0, 3);
    //DrawLine(Buffer, 0, 50, Buffer->Width - 1, 50, 3);
    //DrawLine(Buffer, 0, 150, Buffer->Width - 1, 150, 3);
    //DrawLine(Buffer, 100, 150, 200, 200, 2);
    //DrawLine(Buffer, 100, 150, 200, 150, 5);
    //DrawLine(Buffer, 100, 150, 100, 300, 2);
    //DrawLine(Buffer, 200, 150, 200, 300, 5);
    //DrawLine(Buffer, 100, 300, 200, 300, 5);
    //DrawCircle(Buffer, 30, 30, 15);
    //DrawCircle(Buffer, 300, 300, 100);
    //DrawCircle(Buffer, 300, 300, 100);
    //DrawCircle(Buffer, 300, 300, 100);
    //DrawCircle(Buffer, 300, 300, 100);
    //DrawCircle(Buffer, 300, 300, 100);
    //DrawCircle(Buffer, 300, 300, 100);
    //DrawCircle(Buffer, 300, 300, 100);

    WindowUpdateCallback();
}

#endif //RAYKH
