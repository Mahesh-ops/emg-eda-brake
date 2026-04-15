@echo off
setlocal

echo ========================================================
echo   EMG-EDA Brake Intent Detection - Pipeline Builder
echo ========================================================
echo.

:: 1. Create the build directory if it does not exist
if not exist build (
    echo [*] Creating build directory...
    mkdir build
)

:: 2. Compile the project
echo [*] Compiling C sources...
gcc -Wall -Wextra -Iinclude src\*.c targets\sim\*.c -o build\sim_emg_brake.exe

:: Check if compilation was successful
if %errorlevel% neq 0 (
    echo.
    echo [!] ERROR: Compilation failed. Please check the logs above.
    pause
    exit /b %errorlevel%
)
echo [+] Compilation successful!
echo.

:: 3. Run the simulation
echo ========================================================
echo   Starting Simulation (Scenario: Braking, 30 seconds)
echo ========================================================
echo.

:: Pipe the Python simulator output into the C executable
python tools\stream_simulator.py --scenario braking --duration 30 | build\sim_emg_brake.exe

echo.
echo ========================================================
echo   Simulation Complete
echo ========================================================
pause