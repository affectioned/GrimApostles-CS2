@echo off
setlocal

set VENV_DIR=%~dp0.venv

:: Create venv if it doesn't exist
if not exist "%VENV_DIR%\Scripts\python.exe" (
    echo [setup] Creating virtual environment...
    python -m venv "%VENV_DIR%"
    if errorlevel 1 (
        echo [error] Failed to create venv. Is Python 3.10+ installed and on PATH?
        pause & exit /b 1
    )

    echo [setup] Installing dependencies...
    "%VENV_DIR%\Scripts\pip.exe" install -r "%~dp0requirements.txt"
    if errorlevel 1 (
        echo [error] Dependency installation failed.
        pause & exit /b 1
    )
)

:: Run the script, forwarding all arguments
"%VENV_DIR%\Scripts\python.exe" "%~dp0extract.py" %*
endlocal
pause
