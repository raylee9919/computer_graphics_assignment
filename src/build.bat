@echo off

set CCFLAGS=/std:c++17 /Od /nologo /MT /GR- /EHa- /Oi /W4 /wd4530 /wd4201 /wd4505 /FC /Z7 /D_CRT_SECURE_NO_WARNINGS
set CLFLAGS=/LIBPATH:..\lib opengl32.lib glfw3dll.lib glew32.lib
set CLFLAGS=/incremental:no /opt:ref %CLFLAGS%

if not exist ..\build mkdir ..\build
pushd ..\build

cl %CCFLAGS% ..\src\main.cpp /link %CLFLAGS%
popd
