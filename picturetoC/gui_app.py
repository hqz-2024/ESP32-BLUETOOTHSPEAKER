"""
GUI应用类
"""
from PyQt5.QtWidgets import *
from PyQt5.QtGui import QPixmap
from PyQt5.QtCore import Qt
from PIL import Image
import numpy as np
import os
from image_converter_tool import ImageConverter


class ImageToCApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.img_path = None
        self.img_array = None
        self.orig_img = None
        self.init_ui()
    
    def init_ui(self):
        self.setWindowTitle("图片转C数组工具 v1.0")
        self.setGeometry(100, 100, 1200, 700)
        
        central = QWidget()
        self.setCentralWidget(central)
        layout = QHBoxLayout()
        central.setLayout(layout)
        
        layout.addWidget(self.create_left(), 1)
        layout.addWidget(self.create_right(), 2)
    
    def create_left(self):
        panel = QWidget()
        layout = QVBoxLayout()
        panel.setLayout(layout)
        
        # 导入
        grp1 = QGroupBox("图片导入")
        lay1 = QVBoxLayout()
        self.btn_import = QPushButton("选择图片 (JPG/PNG)")
        self.btn_import.clicked.connect(self.import_image)
        lay1.addWidget(self.btn_import)
        self.lbl_file = QLabel("未选择文件")
        self.lbl_file.setWordWrap(True)
        lay1.addWidget(self.lbl_file)
        grp1.setLayout(lay1)
        layout.addWidget(grp1)
        
        # 预览
        grp2 = QGroupBox("图片预览")
        lay2 = QVBoxLayout()
        self.lbl_preview = QLabel("无预览")
        self.lbl_preview.setAlignment(Qt.AlignCenter)
        self.lbl_preview.setMinimumSize(250, 250)
        self.lbl_preview.setStyleSheet("QLabel{background:#f0f0f0;border:1px solid #ccc;}")
        lay2.addWidget(self.lbl_preview)
        self.lbl_orig_size = QLabel("原始尺寸: -")
        lay2.addWidget(self.lbl_orig_size)
        grp2.setLayout(lay2)
        layout.addWidget(grp2)
        
        # 分辨率
        grp3 = QGroupBox("分辨率设置 (最大640x640)")
        lay3 = QVBoxLayout()
        
        hlay1 = QHBoxLayout()
        hlay1.addWidget(QLabel("宽度:"))
        self.spin_w = QSpinBox()
        self.spin_w.setRange(1, 640)
        self.spin_w.setValue(160)
        hlay1.addWidget(self.spin_w)
        hlay1.addWidget(QLabel("px"))
        lay3.addLayout(hlay1)
        
        hlay2 = QHBoxLayout()
        hlay2.addWidget(QLabel("高度:"))
        self.spin_h = QSpinBox()
        self.spin_h.setRange(1, 640)
        self.spin_h.setValue(160)
        hlay2.addWidget(self.spin_h)
        hlay2.addWidget(QLabel("px"))
        lay3.addLayout(hlay2)
        
        self.btn_apply = QPushButton("应用分辨率")
        self.btn_apply.clicked.connect(self.apply_resolution)
        lay3.addWidget(self.btn_apply)
        
        self.lbl_curr_size = QLabel("当前尺寸: -")
        lay3.addWidget(self.lbl_curr_size)
        grp3.setLayout(lay3)
        layout.addWidget(grp3)
        
        # 格式
        grp4 = QGroupBox("颜色格式")
        lay4 = QVBoxLayout()
        self.combo_fmt = QComboBox()
        self.combo_fmt.addItems([
            "LVGL_TRUE_COLOR (RGBA8888推荐)",
            "CF_TRUE_COLOR (RGB565)",
            "CF_INDEXED_1_BIT", "CF_INDEXED_2_BIT", "CF_INDEXED_4_BIT", "CF_INDEXED_8_BIT",
            "CF_RAW", "CF_RAW_CHROMA", "CF_RAW_ALPHA",
            "CF_TRUE_COLOR_ALPHA", "CF_TRUE_COLOR_CHROMA", "CF_RGB565A8"
        ])
        self.combo_fmt.setCurrentText("LVGL_TRUE_COLOR (RGBA8888推荐)")
        lay4.addWidget(self.combo_fmt)

        lbl_hint = QLabel("提示：LVGL格式会自动生成RGB565数据\n适用于LV_COLOR_DEPTH==16的ESP32项目")
        lbl_hint.setStyleSheet("QLabel{color:#666;font-size:10px;}")
        lay4.addWidget(lbl_hint)

        grp4.setLayout(lay4)
        layout.addWidget(grp4)

        # 字节序选项
        grp5 = QGroupBox("字节序设置 (RGB565格式)")
        lay5 = QVBoxLayout()

        self.radio_big = QRadioButton("大端序 (高字节在前)")
        self.radio_little = QRadioButton("小端序 (低字节在前)")
        self.radio_big.setChecked(True)
        lay5.addWidget(self.radio_big)
        lay5.addWidget(self.radio_little)

        self.chk_swap_rb = QCheckBox("交换红蓝通道 (RGB↔BGR)")
        lay5.addWidget(self.chk_swap_rb)

        grp5.setLayout(lay5)
        layout.addWidget(grp5)

        # 转换
        self.btn_convert = QPushButton("转换为C数组")
        self.btn_convert.clicked.connect(self.convert_to_c)
        self.btn_convert.setStyleSheet("QPushButton{background:#4CAF50;color:white;font-weight:bold;padding:10px;}")
        layout.addWidget(self.btn_convert)
        
        layout.addStretch()
        return panel
    
    def create_right(self):
        panel = QWidget()
        layout = QVBoxLayout()
        panel.setLayout(layout)
        
        grp = QGroupBox("C数组输出")
        lay = QVBoxLayout()
        
        hlay = QHBoxLayout()
        hlay.addWidget(QLabel("数组名称:"))
        self.edit_name = QLineEdit("image_data")
        hlay.addWidget(self.edit_name)
        lay.addLayout(hlay)
        
        self.txt_output = QTextEdit()
        self.txt_output.setReadOnly(True)
        self.txt_output.setPlaceholderText("转换后的C数组将显示在这里...")
        lay.addWidget(self.txt_output)
        
        hlay2 = QHBoxLayout()
        self.btn_copy = QPushButton("复制到剪贴板")
        self.btn_copy.clicked.connect(self.copy_to_clipboard)
        hlay2.addWidget(self.btn_copy)
        
        self.btn_save = QPushButton("保存为.h文件")
        self.btn_save.clicked.connect(self.save_to_file)
        hlay2.addWidget(self.btn_save)
        lay.addLayout(hlay2)
        
        grp.setLayout(lay)
        layout.addWidget(grp)
        
        return panel

