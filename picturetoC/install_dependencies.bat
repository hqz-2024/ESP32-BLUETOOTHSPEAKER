@echo off
title Install Dependencies

echo ========================================
echo Picture to C Array Tool - Install
echo ========================================
echo.
echo Installing dependencies...
echo.

echo [1/3] Installing PyQt5...
pip install --force-reinstall PyQt5==5.15.11

echo.
echo [2/3] Installing Pillow...
pip install Pillow

echo.
echo [3/3] Installing NumPy...
pip install numpy

echo.
echo ========================================
echo Verifying installation...
echo ========================================
python -c "from PyQt5.QtWidgets import QApplication; import PIL; import numpy; print('All dependencies installed successfully!')"

if errorlevel 1 (
    echo.
    echo Installation verification failed!
    echo Please check the error messages above.
) else (
    echo.
    echo ========================================
    echo Installation completed!
    echo ========================================
    echo.
    echo You can now run the tool by double-clicking "start.bat"
)

echo.
pause

