#ifndef RAYKH
#define RAYKH

struct raytracer_memory
{
    bool IsInitialized;
    uint64 PermanentStorageSize;
    void *PermanentStorage;
};


struct renderStats{
    float renderTime;
    int aaSamples;
    int resolution[2];
};


struct raytracer_state
{
    void (*UpdateWindowCallback)(void);
};


struct sunshine_offscreen_buffer
{
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

void UpdateAndRender(raytracer_memory *Memory, sunshine_offscreen_buffer *Buffer, void* callback){

}

#endif //RAYKH
