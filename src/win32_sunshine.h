#ifndef WIN32_RAYKH
#define WIN32_RAYKH

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

struct win32_state
{
    uint64 TotalSize;
    void *RaytracerMemoryBlock;
};

#endif // WIN32_RAYKH
