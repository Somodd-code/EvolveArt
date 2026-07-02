@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
cd /d "%~dp0"
if exist build rmdir /s /q build
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DGEODE_DISABLE_PRECOMPILED_HEADERS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --verbose
