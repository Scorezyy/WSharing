@echo off
setlocal

set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
set CMAKE="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set SRC=e:\Programmierung\WSharing
set BLD=e:\Programmierung\WSharing\build
set OUT=e:\Programmierung\WSharing\output\WSharing.exe

call %VCVARS% >nul 2>&1

if "%1"=="clean" (
    echo Loesche Build-Verzeichnis...
    rmdir /s /q "%BLD%"
)

if not exist "%BLD%\CMakeCache.txt" (
    echo [1/2] CMake konfigurieren...
    %CMAKE% -S "%SRC%" -B "%BLD%" -G Ninja -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 ( echo FEHLER: CMake Konfiguration fehlgeschlagen. & exit /b 1 )
)

echo [2/2] Bauen...
%CMAKE% --build "%BLD%" --config Release
if errorlevel 1 ( echo FEHLER: Build fehlgeschlagen. & exit /b 1 )

echo.
echo Fertig: %OUT%
