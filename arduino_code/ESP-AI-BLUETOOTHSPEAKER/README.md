# ESP32S3 蓝牙音箱程序

基于ESP32S3芯片和8311音频芯片的蓝牙音箱项目。

## 功能特性

1. **蓝牙A2DP接收器**
   - 设备名称：ESP-AI-SPEAKER
   - 无密码连接
   - 自动重连已配对设备
   - 低延时高音质音频传输

2. **I2S音频输出**
   - 支持8311音频芯片
   - 44.1kHz采样率
   - 16位立体声音频
   - 高质量音频输出

3. **音量控制**
   - IO7 ADC电压检测
   - 实时音量调节
   - 防抖动处理

4. **恢复出厂设置**
   - IO0按钮连按5下
   - 清除蓝牙配对记录
   - 自动重启设备

## 硬件连接

### I2S音频输出引脚
```
ESP32S3    8311芯片
IO14   ->  MCK  (Master Clock)
IO15   ->  DOUT (Data Out)
IO16   ->  BCK  (Bit Clock)
IO17   ->  WS   (Word Select)
```

### 其他引脚
```
IO0    ->  BOOT按钮 (恢复出厂设置)
IO7    ->  音量控制ADC输入 (0-3.3V)
```

## 依赖库

确保安装以下库：
- `arduino-audio-tools-1.0.1` (已包含在项目中)
- `ESP32-A2DP` (蓝牙A2DP库)
- `OneButton` (已包含在项目中)
- `Preferences` (ESP32内置)

## 编译和上传

### 使用PlatformIO（推荐）

项目已配置好PlatformIO环境，支持多种ESP32开发板：

```bash
# 编译ESP32经典版本（推荐用于蓝牙音箱）
pio run -e esp32dev

# 上传程序
pio run -e esp32dev -t upload

# 监控串口
pio device monitor -e esp32dev
```

支持的环境：
- `esp32dev` - ESP32经典版本（推荐）
- `nodemcu-32s` - NodeMCU-32S开发板
- `esp32-wroom-32` - ESP32 WROOM-32
- `esp32s3` - ESP32-S3（仅BLE，不推荐用于蓝牙音箱）

### 使用Arduino IDE

1. 安装ESP32开发板支持
2. 安装依赖库
3. 选择对应的开发板
4. 编译上传

## 使用说明

### 首次使用
1. 上传程序到ESP32S3
2. 设备自动进入蓝牙广播模式
3. 手机搜索"ESP-AI-SPEAKER"并连接
4. 连接成功后开始播放音频

### 音量控制
- 通过IO7引脚的ADC电压控制音量
- 电压范围：0V-3.3V
- 音量范围：0%-100%

### 恢复出厂设置
1. 快速连按IO0按钮5次
2. 系统提示"检测到5次点击，执行恢复出厂设置..."
3. 设备自动重启并清除配对记录

## 串口调试信息

程序提供详细的串口调试信息：
- 蓝牙连接状态
- 音频播放状态
- 音量调节信息
- 按钮操作反馈
- 系统状态监控

波特率：115200

## 技术参数

- **芯片**：ESP32S3
- **音频芯片**：8311
- **蓝牙协议**：A2DP
- **音频格式**：16位PCM立体声
- **采样率**：44.1kHz
- **延迟**：低延迟优化
- **音量控制**：12位ADC (0-4095)

## 故障排除

### 蓝牙连接问题
1. 确认设备名称为"ESP-AI-SPEAKER"
2. 清除手机蓝牙缓存
3. 重启ESP32设备
4. 检查串口输出信息

### 音频输出问题
1. 检查I2S引脚连接
2. 确认8311芯片供电正常
3. 检查音频线路连接
4. 查看串口调试信息

### 音量控制问题
1. 检查IO7引脚连接
2. 测量ADC输入电压
3. 确认电压范围在0-3.3V内

## 开发说明

### 修改蓝牙设备名称
```cpp
String deviceName = "ESP-AI-SPEAKER"; // 修改此处
```

### 调整音频参数
```cpp
i2s_config.sample_rate = 44100;       // 采样率
i2s_config.channels = 2;              // 声道数
i2s_config.bits_per_sample = 16;      // 位深度
```

### 修改引脚配置
```cpp
#define I2S_MCK_PIN     14    // Master Clock
#define I2S_DOUT_PIN    15    // Data Out
#define I2S_BCK_PIN     16    // Bit Clock
#define I2S_WS_PIN      17    // Word Select
#define BOOT_BUTTON_PIN 0     // BOOT按钮
#define VOLUME_ADC_PIN  7     // 音量控制ADC
```

## 版本信息

- **版本**：1.0
- **作者**：ESP-AI Team
- **日期**：2024
- **许可证**：开源项目

## 支持

如有问题或建议，请联系开发团队。
