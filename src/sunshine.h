#ifndef RAYKH
#define RAYKH

struct raytracer_memory
{
    bool IsInitialized;
    int offset;
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

void*
GetPixelAddress(sunshine_offscreen_buffer *Buffer, int x, int y)
{
    uint8 *row = (uint8 *)Buffer->Memory;
    int offsetFromOrigin = y * Buffer->Pitch + Buffer->BytesPerPixel * x;
    return row += offsetFromOrigin;
}


void
DrawRectangle(sunshine_offscreen_buffer *Buffer, int x1, int y1, int x2, int y2)
{
    // NOTE(kk): assume for now point A has lower coords
    uint8 Red = 255;
    uint8 Green = 30;
    uint8 Blue = 140;

    uint8 *row = (uint8 *)GetPixelAddress(Buffer, x1, y1);

    for (int y = y1; y < y2; ++y)
    {
        uint32 *Pixel = (uint32 *)row;
        for (int x = x1; x < x2; ++x)
        {
            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
        }
        row += Buffer->Pitch;
    }
}

void
DrawLine(sunshine_offscreen_buffer *Buffer, int x1, int y1, int x2, int y2)
{
    uint8 Red = 255;
    uint8 Blue = 255;

    for (float t = 0.0f; t <= 1.0f; t += 0.0005f)
    {
        float i = x2 * t + (1 - t) * x1;
        float j = y2 * t + (1 - t) * y1;
        uint32 *Pixel = (uint32 *)GetPixelAddress(Buffer, int(i), int(j));
        *Pixel = ((Red << 16) | Blue);
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
        Memory->offset = 0;
        Memory->IsInitialized = 1;
    }

    uint8 *row = (uint8 *)Buffer->Memory;
    for (int y=0; y < Buffer->Height; ++y)
    {
        uint32 *Pixel = (uint32 *)row;

        for (int x=0; x < Buffer->Width; ++x)
        {
            //uint8 Red = 0;
            //uint8 Blue = 0;
            //uint8 Green = 200;
            //*Pixel++ = ((Red << 16 ) | (Green << 8) | Blue);
        }
        row += Buffer->Pitch;
    }

    //DrawRectangle(Buffer, 200, 200, 300, 300);

    WindowUpdateCallback();
}

#endif //RAYKH
