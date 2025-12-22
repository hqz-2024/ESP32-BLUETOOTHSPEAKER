@echo off
chcp 65001 >nul
title 图片转C数组工具

echo ========================================
echo 图片转C数组工具 v1.0
echo ========================================
echo.

python run.py

if errorlevel 1 (
    echo.
    echo 程序运行出错！
    echo 可能原因：
    echo 1. 未安装Python
    echo 2. 未安装依赖库
    echo.
    echo 请先运行 install_dependencies.bat 安装依赖库
    echo.
    pause
)

