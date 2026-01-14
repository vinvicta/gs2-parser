@echo off
REM GS2 Parser Build Script for Windows
REM Usage: build-windows.bat

setlocal EnableDelayedExpansion

set VERSION=1.5.0
set BUILD_DIR=build-release-windows
set INSTALL_DIR=gs2parser-%VERSION%-windows-x64

echo Building GS2 Parser for Windows x64...

REM Check for vcpkg
where vcpkg >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: vcpkg not found in PATH
    echo Please install vcpkg from https://vcpkg.io
    echo.
    echo Then install dependencies:
    echo   vcpkg install cmake bison flex --triplet x64-windows
    pause
    exit /b 1
)

REM Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake not found in PATH
    echo Install CMake or add to PATH
    pause
    exit /b 1
)

REM Clean and create build directory
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure
echo Configuring...
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ..

REM Build
echo Building...
cmake --build . --config Release --parallel

REM Create release package
mkdir "%INSTALL_DIR%"
copy Release\gs2test.exe "%INSTALL_DIR%\"

REM Create README
(
echo GS2 Script Compiler/Disassembler for Windows
echo ==============================================
echo.
echo Compile GS2 source to bytecode:
echo   gs2test.exe script.gs2
echo.
echo Disassemble bytecode to readable format:
echo   gs2test.exe script.gs2bc -d
echo.
echo For more options:
echo   gs2test.exe --help
echo.
echo Documentation: https://github.com/vinvicta/gs2-parser
echo License: MIT
) > "%INSTALL_DIR%\README.txt"

REM Create zip (using PowerShell if available)
echo Creating package...
powershell -Command "Compress-Archive -Path '%INSTALL_DIR%' -DestinationPath 'gs2parser-%VERSION%-windows-x64.zip' -Force" 2>nul

if exist "gs2parser-%VERSION%-windows-x64.zip" (
    echo.
    echo Build complete!
    echo Package: gs2parser-%VERSION%-windows-x64.zip
    echo Location: %CD%
) else (
    echo.
    echo Warning: Could not create zip file
    echo Release files are in: %INSTALL_DIR%
)

pause
