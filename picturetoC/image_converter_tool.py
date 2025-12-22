#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
图片转C数组工具 - 完整版
支持JPG和PNG格式，可调整分辨率（最大640x640），支持多种颜色格式输出
主要使用CF_TRUE_COLOR格式（RGB565）

使用方法: python image_converter_tool.py
依赖库: pip install PyQt5 Pillow numpy
"""

import sys, os
from PyQt5.QtWidgets import *
from PyQt5.QtGui import QPixmap
from PyQt5.QtCore import Qt
from PIL import Image
import numpy as np


class ImageConverter:
    """图片转换器"""
    # 字节序设置：True=大端序(高字节在前), False=小端序(低字节在前)
    BIG_ENDIAN = True
    # RGB/BGR设置：True=交换红蓝通道
    SWAP_RB = False

    @staticmethod
    def rgb888_to_rgb565(r, g, b):
        """将RGB888转换为RGB565"""
        # 如果需要交换红蓝通道
        if ImageConverter.SWAP_RB:
            r, b = b, r
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    
    @staticmethod
    def convert_indexed_1bit(img):
        gray = np.mean(img[:,:,:3], axis=2)
        binary = (gray > 127).astype(np.uint8)
        h, w = binary.shape
        data = []
        for y in range(h):
            for x in range(0, w, 8):
                byte = 0
                for bit in range(8):
                    if x+bit < w and binary[y,x+bit]:
                        byte |= (1 << (7-bit))
                data.append(byte)
        return data
    
    @staticmethod
    def convert_indexed_2bit(img):
        gray = np.mean(img[:,:,:3], axis=2)
        indexed = (gray / 255.0 * 3).astype(np.uint8)
        h, w = indexed.shape
        data = []
        for y in range(h):
            for x in range(0, w, 4):
                byte = 0
                for i in range(4):
                    if x+i < w:
                        byte |= (indexed[y,x+i] << (6-i*2))
                data.append(byte)
        return data
    
    @staticmethod
    def convert_indexed_4bit(img):
        gray = np.mean(img[:,:,:3], axis=2)
        indexed = (gray / 255.0 * 15).astype(np.uint8)
        h, w = indexed.shape
        data = []
        for y in range(h):
            for x in range(0, w, 2):
                byte = (indexed[y,x] << 4)
                if x+1 < w:
                    byte |= indexed[y,x+1]
                data.append(byte)
        return data
    
    @staticmethod
    def convert_indexed_8bit(img):
        gray = np.mean(img[:,:,:3], axis=2).astype(np.uint8)
        return gray.flatten().tolist()
    
    @staticmethod
    def convert_raw(img):
        h, w = img.shape[:2]
        data = []
        for y in range(h):
            for x in range(w):
                data.extend(img[y,x,:3])
        return data
    
    @staticmethod
    def convert_raw_alpha(img):
        h, w = img.shape[:2]
        data = []
        for y in range(h):
            for x in range(w):
                if img.shape[2] >= 4:
                    data.extend(img[y,x,:4])
                else:
                    data.extend([*img[y,x,:3], 255])
        return data
    
    @staticmethod
    def convert_raw_chroma(img):
        return ImageConverter.convert_raw(img)
    
    @staticmethod
    def convert_true_color(img):
        h, w = img.shape[:2]
        data = []
        for y in range(h):
            for x in range(w):
                r, g, b = img[y,x,:3]
                rgb565 = ImageConverter.rgb888_to_rgb565(r, g, b)
                # 根据字节序设置输出
                if ImageConverter.BIG_ENDIAN:
                    data.extend([(rgb565 >> 8) & 0xFF, rgb565 & 0xFF])
                else:
                    data.extend([rgb565 & 0xFF, (rgb565 >> 8) & 0xFF])
        return data
    
    @staticmethod
    def convert_true_color_alpha(img):
        h, w = img.shape[:2]
        data = []
        for y in range(h):
            for x in range(w):
                r, g, b = img[y,x,:3]
                a = img[y,x,3] if img.shape[2] >= 4 else 255
                rgb565 = ImageConverter.rgb888_to_rgb565(r, g, b)
                # 根据字节序设置输出
                if ImageConverter.BIG_ENDIAN:
                    data.extend([(rgb565 >> 8) & 0xFF, rgb565 & 0xFF, a])
                else:
                    data.extend([rgb565 & 0xFF, (rgb565 >> 8) & 0xFF, a])
        return data
    
    @staticmethod
    def convert_true_color_chroma(img):
        return ImageConverter.convert_true_color(img)
    
    @staticmethod
    def convert_rgb565a8(img):
        h, w = img.shape[:2]
        rgb_data, alpha_data = [], []
        for y in range(h):
            for x in range(w):
                r, g, b = img[y,x,:3]
                a = img[y,x,3] if img.shape[2] >= 4 else 255
                rgb565 = ImageConverter.rgb888_to_rgb565(r, g, b)
                # 根据字节序设置输出
                if ImageConverter.BIG_ENDIAN:
                    rgb_data.extend([(rgb565 >> 8) & 0xFF, rgb565 & 0xFF])
                else:
                    rgb_data.extend([rgb565 & 0xFF, (rgb565 >> 8) & 0xFF])
                alpha_data.append(a)
        return rgb_data + alpha_data
    
    @staticmethod
    def convert_lvgl_true_color(img):
        """LVGL TRUE_COLOR格式 - RGBA8888 (每像素4字节)"""
        h, w = img.shape[:2]
        data = []
        for y in range(h):
            for x in range(w):
                r, g, b = img[y,x,:3]
                a = img[y,x,3] if img.shape[2] >= 4 else 255
                # LVGL使用RGBA8888格式
                data.extend([r, g, b, a])
        return data

    @staticmethod
    def convert(img, fmt):
        converters = {
            "CF_INDEXED_1_BIT": ImageConverter.convert_indexed_1bit,
            "CF_INDEXED_2_BIT": ImageConverter.convert_indexed_2bit,
            "CF_INDEXED_4_BIT": ImageConverter.convert_indexed_4bit,
            "CF_INDEXED_8_BIT": ImageConverter.convert_indexed_8bit,
            "CF_RAW": ImageConverter.convert_raw,
            "CF_RAW_CHROMA": ImageConverter.convert_raw_chroma,
            "CF_RAW_ALPHA": ImageConverter.convert_raw_alpha,
            "CF_TRUE_COLOR": ImageConverter.convert_true_color,
            "CF_TRUE_COLOR_ALPHA": ImageConverter.convert_true_color_alpha,
            "CF_TRUE_COLOR_CHROMA": ImageConverter.convert_true_color_chroma,
            "CF_RGB565A8": ImageConverter.convert_rgb565a8,
            "LVGL_TRUE_COLOR": ImageConverter.convert_lvgl_true_color,
        }
        return converters[fmt](img)

