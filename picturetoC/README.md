# 图片转C数组工具

将JPG/PNG图片转换为LVGL格式的C数组，用于ESP32嵌入式开发。

## 功能特点

- 支持JPG和PNG格式图片
- 分辨率调整（最大640x640px）
- 生成LVGL兼容的C数组格式
- 输出格式与ESP32 LVGL项目完全兼容

## 快速开始

### 1. 安装依赖

双击运行 `install_dependencies.bat` 或手动安装：

```bash
conda install -y pillow numpy pyqt -c conda-forge
```

### 2. 启动程序

双击 `启动工具.bat` 或命令行运行：

```bash
python run.py
```

### 3. 使用步骤

1. 点击"导入图片"选择JPG/PNG文件
2. 设置目标分辨率（默认160x160px）
3. 选择"LVGL_TRUE_COLOR"格式
4. 点击"转换为C数组"
5. 点击"保存为.h文件"或"复制到剪贴板"

## 输出格式

生成的C数组格式：

```c
/**
 * image_name - LVGL图片数据
 * 尺寸: 160x160px
 * 格式: RGB565 (16-bit color)
 */

#include <lvgl.h>

const LV_ATTRIBUTE_MEM_ALIGN uint8_t image_name_map[] = {
#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0
  /*Pixel format: Red: 5 bit, Green: 6 bit, Blue: 5 bit*/
  0xff, 0xff, 0xff, 0xff, ...
#endif
};

const lv_img_dsc_t image_name = {
  {
    LV_IMG_CF_TRUE_COLOR,
    0, 0, 160, 160
  },
  25600 * LV_COLOR_SIZE / 8,
  image_name_map,
};
```

## 文件说明

- `run.py` - 主程序入口
- `gui_app.py` - GUI界面
- `gui_methods.py` - 方法实现
- `image_converter_tool.py` - 图片转换核心
- `启动工具.bat` - 快速启动脚本
- `install_dependencies.bat` - 依赖安装脚本

## 技术说明

- **颜色格式**: RGB565 (16位真彩色)
- **字节序**: 小端序
- **兼容性**: ESP32 LVGL项目
- **Python版本**: 3.7+

## 注意事项

- 生成的.h文件可直接在ESP32项目中使用
- 建议图片分辨率为160x160px（常用尺寸）
- 大图片会生成较大的C数组文件

