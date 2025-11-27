# ESP32-BLUETOOTHSPEAKER
基于ESP32的高品质蓝牙A2DP音频接收器

## 项目概述

这是一个模块化设计的ESP32蓝牙音箱项目，支持将手机等蓝牙设备的音频通过I2S接口输出到PCM5102 DAC芯片，实现高保真音频播放。

### 核心特性
- 🎵 **蓝牙A2DP接收** - 支持标准A2DP协议，兼容所有蓝牙音频设备
- 🎧 **高品质音频** - 44.1kHz采样率，16位立体声输出
- 🔊 **智能音量控制** - 电位器实时调节，21档量化控制
- 🔄 **自动重连** - 配对信息持久化，断电自动重连
- 🔧 **恢复出厂** - 按钮5次连击清除配对数据
- 💡 **状态指示** - WS2812 LED显示连接和播放状态
- 📦 **模块化架构** - 清晰代码结构，易于维护扩展

⚠️ **仅支持ESP32经典版本，不支持ESP32-S3**

## 项目结构

```
ESP32-BLUETOOTHSPEAKER/
├── README.md                           # 项目说明
├── arduino_code/
│   └── ESP32-A2DP-SPEAKER/            # 主程序目录
│       ├── main.ino                   # 程序入口
│       ├── userconfig.h               # 硬件配置
│       └── src/                       # 模块源码
│           ├── audio_i2s.h/cpp        # I2S音频处理
│           ├── bluetooth_manager.h/cpp # 蓝牙管理
│           ├── volume_control.h/cpp   # 音量控制
│           ├── led_control.h/cpp      # LED状态指示
│           ├── button_handler.h/cpp   # 按钮处理
│           └── config_manager.h/cpp   # 配置管理
└── arduino_code/libraries2/           # 依赖库目录
```

## 功能模块

### 🎵 音频处理模块 (`audio_i2s.h/cpp`)
- **功能**: I2S硬件配置和音频数据处理
- **核心函数**:
  - `setupI2S()` - 初始化I2S硬件，配置PCM5102
  - `read_data_stream()` - 音频数据流处理回调
  - `setAudioVolume()` - 音量设置
- **特点**: 支持PCM5102 DAC，实时音量调整，低延时传输

### 📡 蓝牙管理模块 (`bluetooth_manager.h/cpp`)
- **功能**: A2DP连接管理和状态回调
- **核心函数**:
  - `initBluetooth()` - 初始化蓝牙A2DP
  - `connection_state_changed()` - 连接状态回调
  - `audio_state_changed()` - 音频状态回调
- **特点**: 自动重连，配对信息持久化，状态事件处理

### 🔊 音量控制模块 (`volume_control.h/cpp`)
- **功能**: ADC电位器音量检测和控制
- **核心函数**:
  - `initVolumeControl()` - 初始化ADC
  - `updateVolumeControl()` - 检测音量变化
  - `getCurrentVolume()` - 获取当前音量
- **特点**: 21档量化控制，防抖处理，实时混合

### 💡 LED控制模块 (`led_control.h/cpp`)
- **功能**: WS2812 LED状态指示
- **核心函数**:
  - `initLedControl()` - 初始化LED
  - `setLedState()` - 设置LED状态
  - `updateLedControl()` - 更新LED显示
- **状态指示**:
  - 🔵 蓝色闪烁 - 等待连接
  - 🔵 蓝色长亮 - 已连接未播放
  - 🟢 绿色呼吸 - 正在播放

### 🔘 按钮处理模块 (`button_handler.h/cpp`)
- **功能**: BOOT按钮事件处理
- **核心函数**:
  - `initButtonHandler()` - 初始化按钮
  - `updateButton()` - 按钮状态检测
  - `handleButtonClick()` - 按钮点击处理
- **特点**: OneButton库，防抖处理，5次连击恢复出厂

### ⚙️ 配置管理模块 (`config_manager.h/cpp`)
- **功能**: 配置参数持久化存储
- **核心函数**:
  - `initConfigManager()` - 初始化配置
  - `saveConfig()` - 保存配置
  - `loadConfig()` - 加载配置
- **特点**: Preferences API，断电保存，恢复出厂设置

### 🔧 硬件配置 (`userconfig.h`)
- **功能**: 集中管理硬件引脚和参数配置
- **配置项**:
  - I2S引脚定义 (BCK, LRCK, DIN, MUTE)
  - 控制引脚 (按钮, ADC, LED)
  - 音频参数 (采样率, 位深度, DMA)
  - 蓝牙设备名称
- **特点**: 修改此文件即可适配不同硬件平台

## 硬件连接

### PCM5102 DAC模块
```
ESP32    →  PCM5102
GPIO33   →  BCK (位时钟)
GPIO26   →  LRCK (左右声道时钟)
GPIO25   →  DIN (数据输入)
GPIO32   →  MUTE (静音控制)
3.3V     →  VIN
GND      →  GND
```

### 可选组件
```
GPIO0    →  BOOT按钮 (恢复出厂设置)
GPIO34   →  音量电位器 (ADC输入)
GPIO2    →  WS2812 LED (状态指示)
```

## 快速开始

### 1. 环境准备
- Arduino IDE 或 PlatformIO
- ESP32开发板 (经典版本)
- PCM5102 DAC模块

### 2. 依赖库安装
```
ESP32-A2DP (latest)
OneButton (latest)
Adafruit_NeoPixel (latest)
```

### 3. 编译配置
```
开发板: ESP32 Dev Module
Flash Size: 4MB
Partition Scheme: Default 4MB with spiffs
CPU Frequency: 240MHz
```

### 4. 使用步骤
1. 按接线图连接硬件
2. 编译上传程序
3. 手机搜索"ESP-AI-SPEAKER"
4. 配对连接后播放音乐

## 事件驱动架构

### 蓝牙音频事件
- **架构**: 回调函数事件驱动
- **流程**: `ESP32硬件 → ESP-IDF → C++类 → 用户回调`
- **特点**: 实时处理，低延时，异步非阻塞

### 按钮事件
- **架构**: OneButton轮询式事件驱动
- **特点**: 防抖处理，多击检测，抗干扰
- **优势**: 避免音频干扰，逻辑清晰

### IO状态检测
- **方法**: 位异或运算检测变化
- **代码**: `uint8_t changed = currentState ^ lastIOState`
- **优势**: 高效，精确，适合多IO监控

## 技术特点

- **纯A2DP实现** - 不依赖AudioTools，减少内存占用
- **FreeRTOS架构** - Arduino + FreeRTOS混合架构
- **模块化设计** - 功能独立，易于维护扩展
- **配对持久化** - 使用Preferences API存储配置
- **实时音量混合** - 手机音量 × 电位器音量

## 故障排除

### 无法连接蓝牙
- 确认使用ESP32经典版本
- 恢复出厂设置清除配对
- 重启ESP32和手机蓝牙

### 音质失真
- 降低音量避免过载
- 使用独立稳压电源
- 缩短I2S信号线长度

### 恢复出厂设置
快速连续按BOOT按钮5次，清除所有配对数据

## 开发团队

**ESP-AI Team** @ 2024
