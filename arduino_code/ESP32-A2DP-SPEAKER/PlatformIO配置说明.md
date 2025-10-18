# PlatformIO 配置说明

## 环境配置

项目包含多个环境配置，适用于不同的ESP32芯片：

### 1. ESP32S3 环境 (esp32s3)
```ini
[env:esp32s3]
```
- **适用芯片**: ESP32-S3
- **蓝牙支持**: 仅BLE，不支持A2DP
- **推荐用途**: BLE应用，不适合蓝牙音箱
- **源码文件**: `ESP32S3-BLE-AUDIO.INO`

### 2. ESP32经典版本环境 (esp32dev) - **推荐**
```ini
[env:esp32dev]
```
- **适用芯片**: ESP32经典版本
- **蓝牙支持**: 完整支持A2DP
- **推荐用途**: 蓝牙音箱项目
- **源码文件**: `ESP32-A2DP-SPEAKER.INO`

### 3. NodeMCU-32S环境 (nodemcu-32s)
```ini
[env:nodemcu-32s]
```
- **适用芯片**: NodeMCU-32S开发板
- **蓝牙支持**: 完整支持A2DP
- **推荐用途**: 蓝牙音箱项目
- **源码文件**: `ESP32-A2DP-SPEAKER.INO`

### 4. ESP32 WROOM-32环境 (esp32-wroom-32)
```ini
[env:esp32-wroom-32]
```
- **适用芯片**: ESP32 WROOM-32
- **蓝牙支持**: 完整支持A2DP
- **推荐用途**: 蓝牙音箱项目
- **源码文件**: `ESP32-A2DP-SPEAKER.INO`

## 使用方法

### 1. 选择合适的环境

根据您的硬件选择对应的环境：

```bash
# ESP32经典版本（推荐用于蓝牙音箱）
pio run -e esp32dev

# NodeMCU-32S开发板
pio run -e nodemcu-32s

# ESP32 WROOM-32
pio run -e esp32-wroom-32

# ESP32S3（仅BLE功能）
pio run -e esp32s3
```

### 2. 上传程序

```bash
# 上传到ESP32经典版本
pio run -e esp32dev -t upload

# 上传到NodeMCU-32S
pio run -e nodemcu-32s -t upload
```

### 3. 监控串口

```bash
# 监控ESP32经典版本
pio device monitor -e esp32dev

# 监控NodeMCU-32S
pio device monitor -e nodemcu-32s
```

## 关键配置说明

### 蓝牙A2DP编译标志
```ini
build_flags = 
    -DCONFIG_BT_ENABLED=1
    -DCONFIG_BLUEDROID_ENABLED=1
    -DCONFIG_BT_A2DP_ENABLE=1
    -DCONFIG_BT_SPP_ENABLED=1
```

### 依赖库
```ini
lib_deps = 
    https://github.com/pschatzmann/ESP32-A2DP.git
    https://github.com/pschatzmann/arduino-audio-tools.git
    mathertel/OneButton@^2.0.3
```

### 分区表配置
```ini
board_build.partitions = huge_app.csv
```
- 为蓝牙功能预留更多应用空间

## 源码文件对应关系

| 环境名称 | 芯片类型 | 推荐源码文件 | 蓝牙功能 |
|---------|---------|-------------|----------|
| esp32dev | ESP32经典 | ESP32-A2DP-SPEAKER.INO | 完整A2DP |
| nodemcu-32s | NodeMCU-32S | ESP32-A2DP-SPEAKER.INO | 完整A2DP |
| esp32-wroom-32 | WROOM-32 | ESP32-A2DP-SPEAKER.INO | 完整A2DP |
| esp32s3 | ESP32-S3 | ESP32S3-BLE-AUDIO.INO | 仅BLE |

## 项目结构

```
arduino_code/ESP-AI-BLUETOOTHSPEAKER/
├── ESP32-A2DP-SPEAKER.INO          # ESP32经典版本A2DP音箱（推荐）
├── ESP32S3-BLE-AUDIO.INO           # ESP32S3 BLE音频方案
├── ESP-AI-BLUETOOTHSPEAKER.INO     # 原始文件（需要修改）
├── README.md                       # 项目说明
├── PlatformIO配置说明.md           # 本文件
└── ESP32S3-蓝牙音频解决方案.md     # 技术方案说明
```

## 推荐配置

### 蓝牙音箱项目推荐配置：

1. **硬件**: ESP32经典版本（非S3）
2. **环境**: `esp32dev` 或 `nodemcu-32s`
3. **源码**: `ESP32-A2DP-SPEAKER.INO`
4. **编译命令**: `pio run -e esp32dev`
5. **上传命令**: `pio run -e esp32dev -t upload`

### 如果必须使用ESP32S3：

1. **硬件**: ESP32-S3
2. **环境**: `esp32s3`
3. **源码**: `ESP32S3-BLE-AUDIO.INO`
4. **编译命令**: `pio run -e esp32s3`
5. **注意**: 需要专用手机APP，兼容性有限

## 故障排除

### 编译错误
1. 确保选择了正确的环境
2. 检查依赖库是否正确安装
3. 确认源码文件与环境匹配

### 上传失败
1. 检查串口连接
2. 确认开发板型号
3. 尝试降低上传速度：`upload_speed = 115200`

### 蓝牙连接问题
1. 确认使用ESP32经典版本（非S3）
2. 检查蓝牙编译标志是否正确
3. 查看串口调试信息

## 注意事项

1. **ESP32S3不支持A2DP**：如果需要蓝牙音箱功能，请使用ESP32经典版本
2. **分区表**：蓝牙功能需要较大的应用空间，建议使用`huge_app.csv`
3. **内存**：蓝牙A2DP功能消耗较多内存，建议使用有PSRAM的开发板
4. **调试**：使用`monitor_filters = esp32_exception_decoder`获得更好的调试信息

## 更多信息

- [ESP32-A2DP库文档](https://github.com/pschatzmann/ESP32-A2DP)
- [Arduino Audio Tools文档](https://github.com/pschatzmann/arduino-audio-tools)
- [PlatformIO ESP32平台文档](https://docs.platformio.org/en/latest/platforms/espressif32.html)
