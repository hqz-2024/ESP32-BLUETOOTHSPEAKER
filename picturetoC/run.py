#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
图片转C数组工具 - 主程序入口
支持JPG和PNG格式，可调整分辨率（最大640x640），支持多种颜色格式输出
主要使用CF_TRUE_COLOR格式（RGB565）

使用方法:
    python run.py

依赖库:
    pip install PyQt5 Pillow numpy
"""

import sys
import os

# 设置Qt平台插件路径（修复Windows下的Qt插件加载问题）
if sys.platform == 'win32':
    # 尝试设置Qt插件路径
    try:
        import PyQt5
        pyqt5_path = os.path.dirname(PyQt5.__file__)
        plugins_path = os.path.join(pyqt5_path, 'Qt5', 'plugins')
        if os.path.exists(plugins_path):
            os.environ['QT_PLUGIN_PATH'] = plugins_path
    except:
        pass

from PyQt5.QtWidgets import QApplication
from gui_app import ImageToCApp
from gui_methods import bind_methods


def main():
    """主函数"""
    # 绑定方法到GUI类
    bind_methods(ImageToCApp)
    
    # 创建应用
    app = QApplication(sys.argv)
    
    # 创建主窗口
    window = ImageToCApp()
    window.show()
    
    # 运行应用
    sys.exit(app.exec_())


if __name__ == '__main__':
    main()

