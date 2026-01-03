@echo off
REM Windows build script for MemoryDB

echo ====================================
echo MemoryDB Build Script
echo ====================================
echo.

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo ====================================
echo Build completed successfully!
echo Binaries are in: build\bin\
echo ====================================

cd ..
