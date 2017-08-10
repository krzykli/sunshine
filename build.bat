@echo off

IF NOT "%VisualStudioVersion%"=="14.0" ^
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64

REM 4239 - nonstandard extension, (a non-const reference may only be bount ot an lvalue)
    set CompilerFlags=-MT -Oi -WX -W4 -wd4101 -wd4239 -wd4189 -wd4100 -wd4530 -FC -Zi -EHs- -Wv:18
REM 4100 - unreferenced formal parameter
REM 4101 - unreferenced local variable
REM 4189 - unused variable
REM 4530 - somebody used exception handling
REM MT - statically link c runtime library
REM Oi - enable intrinsics
REM FC - display pull path passed to cl
REM Zi - generate debug info
REM EHs- - turn off exception handling
REM Wv- - no warnings after compiler version

IF NOT EXIST bin mkdir bin
pushd bin
cl %CompilerFlags% /DDEBUG ..\src\win32_sunshine.cpp gdi32.lib user32.lib /link -subsystem:windows /out:sunshine.exe
popd

