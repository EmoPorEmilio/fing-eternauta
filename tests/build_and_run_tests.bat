@echo off
REM ============================================================================
REM Math Unit Tests Build and Run Script
REM ============================================================================
REM
REM This script compiles and runs the math unit tests for the ECS engine.
REM
REM Prerequisites:
REM   - Visual Studio 2022 with C++ workload installed
REM   - Run from the tests directory or the project root
REM
REM Usage:
REM   build_and_run_tests.bat
REM
REM ============================================================================

setlocal

REM Find the script directory
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

echo.
echo ============================================================================
echo  Math Unit Tests - Build and Run
echo ============================================================================
echo.

REM Try to find Visual Studio
set "VS_PATH="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VS_PATH%"=="" (
    echo ERROR: Could not find Visual Studio 2022
    echo Please ensure Visual Studio 2022 is installed with C++ workload
    exit /b 1
)

echo Found Visual Studio at: %VS_PATH%
echo.

REM Initialize Visual Studio environment
call "%VS_PATH%"
if errorlevel 1 (
    echo ERROR: Failed to initialize Visual Studio environment
    exit /b 1
)

echo.
echo Compiling tests...
echo.

REM Compile the test executable
cl.exe /nologo /EHsc /std:c++17 /W4 ^
    /I".." ^
    /I"..\libraries\glm" ^
    MathTests.cpp ^
    /Fe:MathTests.exe ^
    /Fo:MathTests.obj

if errorlevel 1 (
    echo.
    echo ============================================================================
    echo  BUILD FAILED
    echo ============================================================================
    exit /b 1
)

echo.
echo Build successful! Running tests...
echo.

REM Run the tests
MathTests.exe
set TEST_RESULT=%errorlevel%

REM Cleanup
del /q MathTests.obj 2>nul

echo.
if %TEST_RESULT%==0 (
    echo ============================================================================
    echo  ALL TESTS PASSED
    echo ============================================================================
) else (
    echo ============================================================================
    echo  SOME TESTS FAILED
    echo ============================================================================
)

exit /b %TEST_RESULT%
