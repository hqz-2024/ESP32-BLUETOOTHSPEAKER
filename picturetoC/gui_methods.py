"""
GUI方法实现
"""
from PyQt5.QtWidgets import QMessageBox, QFileDialog, QApplication
from PyQt5.QtGui import QPixmap
from PyQt5.QtCore import Qt
from PIL import Image
import numpy as np
import os
from image_converter_tool import ImageConverter


def import_image(self):
    path, _ = QFileDialog.getOpenFileName(self, "选择图片", "", "图片文件 (*.jpg *.jpeg *.png);;所有文件 (*.*)")
    if path:
        try:
            img = Image.open(path)
            if img.mode != 'RGBA':
                img = img.convert('RGBA')

            self.img_path = path
            self.orig_img = img

            # 获取原始尺寸
            w, h = img.size
            self.lbl_orig_size.setText(f"原始尺寸: {w}x{h}")

            # 使用当前设置的分辨率（默认160x160），不跟随图片变化
            target_w = self.spin_w.value()
            target_h = self.spin_h.value()

            # 调整图片到目标分辨率
            resized = img.resize((target_w, target_h), Image.Resampling.LANCZOS)
            self.img_array = np.array(resized)

            self.lbl_file.setText(f"文件: {os.path.basename(path)}")
            self.lbl_curr_size.setText(f"当前尺寸: {target_w}x{target_h}")

            self.update_preview()
            # QMessageBox.information(self, "成功", f"图片导入成功!\n已自动调整为 {target_w}x{target_h}")
        except Exception as e:
            QMessageBox.critical(self, "错误", f"无法加载图片: {str(e)}")


def apply_resolution(self):
    if self.orig_img is None:
        QMessageBox.warning(self, "警告", "请先导入图片!")
        return
    
    try:
        w = self.spin_w.value()
        h = self.spin_h.value()
        
        resized = self.orig_img.resize((w, h), Image.Resampling.LANCZOS)
        self.img_array = np.array(resized)
        
        self.lbl_curr_size.setText(f"当前尺寸: {w}x{h}")
        self.update_preview()
        
        QMessageBox.information(self, "成功", f"分辨率已调整为 {w}x{h}")
    except Exception as e:
        QMessageBox.critical(self, "错误", f"调整分辨率失败: {str(e)}")


def update_preview(self):
    if self.img_array is None:
        return
    
    try:
        img = Image.fromarray(self.img_array)
        temp = "temp_preview.png"
        img.save(temp)
        pixmap = QPixmap(temp)
        scaled = pixmap.scaled(250, 250, Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.lbl_preview.setPixmap(scaled)
        
        if os.path.exists(temp):
            os.remove(temp)
    except Exception as e:
        print(f"预览更新失败: {str(e)}")


def convert_to_c(self):
    if self.img_array is None:
        QMessageBox.warning(self, "警告", "请先导入图片!")
        return

    try:
        # 设置字节序和RGB/BGR选项
        ImageConverter.BIG_ENDIAN = self.radio_big.isChecked()
        ImageConverter.SWAP_RB = self.chk_swap_rb.isChecked()

        fmt_display = self.combo_fmt.currentText()
        # 提取实际格式名称（去掉括号中的说明）
        fmt = fmt_display.split()[0]

        name = self.edit_name.text().strip()
        if not name:
            name = "image_data"

        data = ImageConverter.convert(self.img_array, fmt)

        h, w = self.img_array.shape[:2]

        # 添加字节序信息到注释（仅对RGB565格式）
        if "RGB565" in fmt or "TRUE_COLOR" in fmt and "LVGL" not in fmt:
            endian_str = "大端序" if ImageConverter.BIG_ENDIAN else "小端序"
            swap_str = " (BGR)" if ImageConverter.SWAP_RB else " (RGB)"
        else:
            endian_str = ""
            swap_str = ""

        c_text = format_c_array(data, name, w, h, fmt_display, endian_str, swap_str)

        self.txt_output.setPlainText(c_text)
        
        QMessageBox.information(self, "成功", f"转换成功!\n格式: {fmt}\n数组大小: {len(data)} 字节")
    except Exception as e:
        QMessageBox.critical(self, "错误", f"转换失败: {str(e)}")


def format_c_array(data, name, w, h, fmt, endian_str="", swap_str=""):
    """格式化C数组输出"""
    lines = []

    # 检查是否是LVGL格式
    is_lvgl = "LVGL" in fmt

    if is_lvgl:
        # LVGL格式头部 - 匹配xingkong.h格式
        lines.append("/**")
        lines.append(f" * {name} - LVGL图片数据")
        lines.append(" *")
        lines.append(f" * 尺寸: {w}x{h}px")
        lines.append(" * 格式: RGB565 (16-bit color)")
        lines.append(" *")
        lines.append(" * 由图片转C数组工具生成")
        lines.append(" */")
        lines.append("")
        lines.append("#include <lvgl.h>")
        lines.append("")
        lines.append("#ifndef LV_ATTRIBUTE_MEM_ALIGN")
        lines.append("#define LV_ATTRIBUTE_MEM_ALIGN")
        lines.append("#endif")
        lines.append("")
        lines.append(f"#ifndef LV_ATTRIBUTE_IMG_{name.upper()}")
        lines.append(f"#define LV_ATTRIBUTE_IMG_{name.upper()}")
        lines.append("#endif")
        lines.append("")

        # 生成RGB565格式数据（用于16位颜色深度）
        # 从RGBA8888转换为RGB565
        rgb565_data = []
        for i in range(0, len(data), 4):
            r, g, b = int(data[i]), int(data[i+1]), int(data[i+2])
            # RGB565转换
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            # 小端序：低字节在前
            rgb565_data.append(rgb565 & 0xFF)
            rgb565_data.append((rgb565 >> 8) & 0xFF)

        # 数组声明
        lines.append(f"const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_{name.upper()} uint8_t {name}_map[] = {{")

        # 输出RGB565数据（LV_COLOR_DEPTH == 16）
        lines.append("#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0")
        lines.append("  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit*/")
        for i in range(0, len(rgb565_data), 16):
            chunk = rgb565_data[i:i+16]
            hex_vals = [f"0x{b:02x}" for b in chunk]
            line = "  " + ", ".join(hex_vals)
            if i + 16 < len(rgb565_data):
                line += ","
            lines.append(line)
        lines.append("#endif")
        lines.append("};")
        lines.append("")

        # lv_img_dsc_t结构体 - 匹配xingkong.h格式
        lines.append(f"const lv_img_dsc_t {name} = {{")
        lines.append("  {")
        lines.append("    LV_IMG_CF_TRUE_COLOR,  // cf (color format)")
        lines.append("    0,                            // always_zero")
        lines.append("    0,                            // reserved")
        lines.append(f"    {w},                          // w (width)")
        lines.append(f"    {h}                           // h (height)")
        lines.append("  },")
        lines.append(f"  {w * h} * LV_COLOR_SIZE / 8,  // data_size")
        lines.append(f"  {name}_map,                      // data")
        lines.append("};")

    else:
        # 普通格式
        lines.append(f"// {w}x{h} {fmt}")
        if endian_str:
            lines.append(f"// 字节序: {endian_str}{swap_str}")
        lines.append(f"const unsigned char {name}[] = {{")

        # 数据部分
        indent = "    "
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_vals = [f"0x{b:02x}" for b in chunk]
            line = indent + ", ".join(hex_vals)
            if i + 16 < len(data):
                line += ","
            lines.append(line)

        lines.append("};")
        lines.append(f"// 数组大小: {len(data)} 字节")

    return "\n".join(lines)


def copy_to_clipboard(self):
    text = self.txt_output.toPlainText()
    if not text:
        QMessageBox.warning(self, "警告", "没有可复制的内容!")
        return
    
    QApplication.clipboard().setText(text)
    QMessageBox.information(self, "成功", "已复制到剪贴板!")


def save_to_file(self):
    text = self.txt_output.toPlainText()
    if not text:
        QMessageBox.warning(self, "警告", "没有可保存的内容!")
        return
    
    path, _ = QFileDialog.getSaveFileName(self, "保存为.h文件", "", "头文件 (*.h);;所有文件 (*.*)")
    
    if path:
        try:
            if not path.endswith('.h'):
                path += '.h'
            
            guard = os.path.basename(path).replace('.', '_').upper()
            
            with open(path, 'w', encoding='utf-8') as f:
                f.write(f"#ifndef {guard}\n")
                f.write(f"#define {guard}\n\n")
                f.write(text)
                f.write(f"\n\n#endif // {guard}\n")
            
            QMessageBox.information(self, "成功", f"文件已保存到:\n{path}")
        except Exception as e:
            QMessageBox.critical(self, "错误", f"保存文件失败: {str(e)}")


# 将方法绑定到类
def bind_methods(cls):
    cls.import_image = import_image
    cls.apply_resolution = apply_resolution
    cls.update_preview = update_preview
    cls.convert_to_c = convert_to_c
    cls.copy_to_clipboard = copy_to_clipboard
    cls.save_to_file = save_to_file

