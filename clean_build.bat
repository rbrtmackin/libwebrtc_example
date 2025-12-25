@echo off
echo ========================================
echo CLEAN BUILD - Removing ALL cache files
echo ========================================

REM Delete build directory
if exist build (
    echo Deleting build directory...
    rmdir /s /q build
)

REM Delete any obj files
echo Deleting object files...
del /s /q *.obj 2>nul

REM Delete CMake cache
del /s /q CMakeCache.txt 2>nul
del /s /q cmake_install.cmake 2>nul

REM Delete Visual Studio user files
del /s /q *.user 2>nul
del /s /q *.vcxproj.filters 2>nul

echo.
echo ========================================
echo Starting FRESH build
echo ========================================
echo.

REM Create build directory
mkdir build
cd build

REM Run CMake
cmake .. -G "Visual Studio 17 2022" -A x64 -DWEBRTC_ROOT=C:/webrtc-prebuilt

if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Building project
echo ========================================
echo.

REM Build
cmake --build . --config Release --clean-first

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo.
echo Executable: build\bin\Release\webrtc_server.exe
echo.

cd ..
pause
