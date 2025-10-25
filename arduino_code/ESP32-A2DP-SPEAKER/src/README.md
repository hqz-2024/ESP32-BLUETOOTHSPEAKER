# ESP32 A2DP音箱模块化架构说明

## 概述

本项目采用模块化设计，将原来的单一.INO文件拆分为多个功能模块，便于维护、扩展和适配不同硬件平台。

## 模块结构

### 1. userconfig.h - 硬件配置文件
**功能：** 集中管理所有硬件相关的引脚定义和配置参数

**包含内容：**
- I2S音频输出引脚配置（BCK, LRCK, DIN, MUTE）
- 控制引脚配置（按钮、ADC、LED）
- I2S音频参数（采样率、位深度、DMA配置）
- 音量控制参数
- 按钮控制参数
- LED控制参数
- 蓝牙配置参数
- LED颜色定义

**适配其他硬件：** 只需修改此文件中的引脚定义即可适配不同的硬件平台

### 2. audio_i2s.h/cpp - I2S音频处理模块
**功能：** 负责I2S硬件配置和音频数据处理

**主要函数：**
- `setupI2S()` - 初始化I2S硬件，配置PCM5102 DAC芯片
- `read_data_stream()` - 音频数据流处理回调，实现音量控制
- `setAudioVolume()` - 设置音量
- `getAudioVolume()` - 获取当前音量

**特点：**
- 支持PCM5102 DAC芯片
- 实时音量调整
- 低延时音频传输

### 3. bluetooth_manager.h/cpp - 蓝牙管理模块
**功能：** 负责蓝牙A2DP连接管理和状态回调

**主要函数：**
- `initBluetooth()` - 初始化蓝牙A2DP接收器
- `getA2DPSink()` - 获取A2DP Sink对象指针
- `connection_state_changed()` - 连接状态回调
- `audio_state_changed()` - 音频播放状态回调
- `isBluetoothConnected()` - 获取连接状态
- `isAudioPlaying()` - 获取播放状态
- `factoryReset()` - 恢复出厂设置

**特点：**
- 自动重连功能
- 配对信息管理
- 状态监控

### 4. volume_control.h/cpp - 音量控制模块
**功能：** 负责ADC音量检测和音量管理

**主要函数：**
- `initVolumeControl()` - 初始化音量控制
- `updateVolume()` - 从ADC读取并更新音量
- `getCurrentVolume()` - 获取当前音量

**特点：**
- 12位ADC精度
- 音量量化（21档位）
- 防抖动处理

### 5. led_control.h/cpp - LED控制模块
**功能：** 负责WS2812 RGB LED状态指示

**主要函数：**
- `initLedControl()` - 初始化LED控制
- `updateRgbLed()` - 更新LED状态显示

**LED状态指示：**
- 未连接：蓝色闪烁（1秒间隔）
- 已连接未播放：蓝色长亮
- 播放中：绿色呼吸灯效果

### 6. button_handler.h/cpp - 按钮处理模块
**功能：** 负责按钮事件检测和处理

**主要函数：**
- `initButtonHandler()` - 初始化按钮处理
- `updateButton()` - 更新按钮状态
- `checkMultiClickTimeout()` - 检查多击超时

**特点：**
- 支持多击检测
- 连按5下恢复出厂设置
- 防抖动处理

### 7. config_manager.h/cpp - 配置管理模块
**功能：** 负责蓝牙配对信息的保存和加载

**主要函数：**
- `saveBluetoothConfig()` - 保存蓝牙配置
- `loadBluetoothConfig()` - 加载蓝牙配置
- `clearBluetoothConfig()` - 清除蓝牙配置

**特点：**
- 使用ESP32 Preferences存储
- 持久化配对信息

## 主程序结构

主程序（ESP32-A2DP-SPEAKER.INO）现在非常简洁：

```cpp
// 包含所有功能模块
#include "src/userconfig.h"
#include "src/audio_i2s.h"
#include "src/bluetooth_manager.h"
#include "src/volume_control.h"
#include "src/led_control.h"
#include "src/button_handler.h"
#include "src/config_manager.h"

void setup() {
  // 初始化各个功能模块
  initVolumeControl();
  initLedControl();
  initButtonHandler();
  setupI2S();
  initBluetooth(BT_DEVICE_NAME);
  getA2DPSink()->set_stream_reader(read_data_stream, false);
}

void loop() {
  // 更新各个模块
  updateButton();
  checkMultiClickTimeout();
  updateVolume();
  updateRgbLed(isBluetoothConnected(), isAudioPlaying());
}
```

## 模块化优势

1. **易于维护**：每个模块职责单一，代码清晰
2. **易于扩展**：添加新功能只需创建新模块
3. **易于测试**：可以单独测试每个模块
4. **易于适配**：修改userconfig.h即可适配不同硬件
5. **代码复用**：模块可以在其他项目中复用
6. **降低耦合**：模块之间通过接口通信，降低耦合度

## 如何适配其他硬件

1. 打开 `src/userconfig.h`
2. 修改引脚定义部分：
   ```cpp
   #define I2S_BCK_PIN     33    // 修改为你的BCK引脚
   #define I2S_LRCK_PIN    26    // 修改为你的LRCK引脚
   #define I2S_DIN_PIN     25    // 修改为你的DIN引脚
   // ... 其他引脚
   ```
3. 根据需要调整其他参数（音量、LED亮度等）
4. 重新编译上传

## 依赖库

- ESP32-A2DP (BluetoothA2DPSink)
- OneButton
- Preferences
- Adafruit_NeoPixel

## 编译说明

使用Arduino IDE或PlatformIO编译时，确保所有模块文件都在src目录下，主程序会自动包含它们。

## 作者

ESP-AI Team @ 2024

