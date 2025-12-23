@echo off
title Picture to C Array Tool

echo ========================================
echo Picture to C Array Tool v1.0
echo ========================================
echo.
echo Starting...
echo.

python run.py

if errorlevel 1 (
    echo.
    echo ========================================
    echo Error: Program failed to start!
    echo ========================================
    echo.
    echo Possible reasons:
    echo 1. Python not installed
    echo 2. Missing dependencies (PyQt5, Pillow, NumPy)
    echo 3. PyQt5 installation corrupted
    echo.
    echo Solution:
    echo 1. Run "install_dependencies.bat" to install/fix dependencies
    echo 2. Or manually run: pip install --force-reinstall PyQt5==5.15.11
    echo.
    pause
)

