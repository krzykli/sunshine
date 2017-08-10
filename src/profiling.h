
global_variable LARGE_INTEGER _PCFreq;

LARGE_INTEGER
GetCurrentClockCounter()
{
     LARGE_INTEGER Result;
     QueryPerformanceCounter(&Result);
     return Result;
}

float
GetMilisecondsElapsed(LARGE_INTEGER StartTime, LARGE_INTEGER EndTime)
{
    LARGE_INTEGER TimeElapsed;
    TimeElapsed.QuadPart = (
        (EndTime.QuadPart - StartTime.QuadPart) * 1000 / _PCFreq.QuadPart);
    return (float)TimeElapsed.QuadPart;
}

void PrintTime(float timeElapsed, char * label)
{
    char msPerFrameBuffer[512];
    int FPS = (int)(1 / (timeElapsed / 1000.0));
    sprintf_s(msPerFrameBuffer, "[%s] \t\t\t %.06f ms/f \t %i FPS\n",
              label,
              timeElapsed,
              FPS);
    OutputDebugStringA(msPerFrameBuffer);
}
