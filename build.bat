@echo off
REM Quick build script for WebRTC Server with Web Client
REM Requires: Visual Studio 2019 or 2022, CMake 3.15+

echo ========================================
echo WebRTC Server - Quick Build Script
echo ========================================
echo.

REM Check if WebRTC path is set
if not defined WEBRTC_ROOT (
    set WEBRTC_ROOT=C:\webrtc-prebuilt
    echo Using default WebRTC path: %WEBRTC_ROOT%
) else (
    echo Using WebRTC path: %WEBRTC_ROOT%
)

REM Check if WebRTC exists
if not exist "%WEBRTC_ROOT%\include" (
    echo ERROR: WebRTC not found at %WEBRTC_ROOT%
    echo.
    echo Please download bengreenier/webrtc from:
    echo https://github.com/bengreenier/webrtc/releases/latest
    echo.
    echo Extract to: %WEBRTC_ROOT%
    pause
    exit /b 1
)

echo WebRTC found!
echo.

REM Create build directory
if not exist build mkdir build
cd build

echo Running CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DWEBRTC_ROOT=%WEBRTC_ROOT%

if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed
    echo.
    echo Trying Visual Studio 2019...
    cmake .. -G "Visual Studio 16 2019" -A x64 -DWEBRTC_ROOT=%WEBRTC_ROOT%
    
    if errorlevel 1 (
        echo.
        echo ERROR: CMake failed. Please install Visual Studio 2019 or 2022
        pause
        exit /b 1
    )
)

echo.
echo Building Release configuration...
cmake --build . --config Release

if errorlevel 1 (
    echo.
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build Successful!
echo ========================================
echo.
echo Executable: bin\Release\webrtc_server.exe
echo HTML Client: bin\client.html
echo.
echo To run:
echo   cd bin\Release
echo   webrtc_server.exe
echo.
echo Then open in browser: http://localhost:8080/
echo.
pause
