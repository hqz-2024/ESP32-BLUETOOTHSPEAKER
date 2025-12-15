# SerialScreen 库

SerialScreen 是一个用于 Arduino 平台的串口显示屏控制库，提供简单易用的接口来控制串口显示屏上的各种元素，包括文本、图标、动画和情绪状态显示。

## 功能特性

- 支持多屏幕切换
- 文本内容更新和显示控制
- 图标显示（电池电量、音量、信号强度等）
- 动画播放控制（显示/隐藏、播放/停止）
- 情绪状态渲染功能（基于GIF动画）
- 支持简单模式

## 控制元素说明

串口屏使用三种基本控制元素类型，每个控制元素都有两个基本属性（屏幕ID和控制ID），用于操作相应的控制元素：

1. **动画控制元素**：主要用于显示情绪GIF动画
2. **图标控制元素**：用于显示电池电量、音量、信号强度等状态图标
3. **文本控制元素**：用于显示各种文本信息

关于控制元素的详细说明，请参考《震丰串口屏操作指南6.4.CHM》的第3章。

## 快速开始

### 初始化

```cpp
#include <serial_screen.h>

// 创建情绪映射表
EmotionControlPair emotionMap[] = {
  {"休息中", CONTROL_SLEEP_ANIMATION},
  {"聆听中", CONTROL_LISTEN_ANIMATION},
  {"对话中", CONTROL_ING_ANIMATION},
  {"开心", CONTROL_HAPPY_ANIMATION},
  {"意外", CONTROL_ACCIDENT_ANIMATION},
  {"音乐", CONTROL_MUSIC_ANIMATION},
  {"生气", CONTROL_ANGRY_ANIMATION},
  {"拒绝", CONTROL_NO_ANIMATION},
  {"悲伤", CONTROL_SAD_ANIMATION}
};

// 创建SerialScreen实例
SerialScreen serialScreen(&Serial2, emotionMap, sizeof(emotionMap) / sizeof(EmotionControlPair));

void setup() {
  // 初始化串口屏通信
  serialScreen.begin(115200);
  
  // 切换到主屏幕
  serialScreen.switchScreen(SCREEN_MAIN);
}
```

## 核心API

### 屏幕控制

- **`switchScreen(uint16_t screenId)`**：切换显示屏幕
  - `screenId`：屏幕ID，用于指定要切换到的界面

### 文本控制

- **`updateText(uint16_t screenId, uint16_t controlId, const char* text)`**：更新指定控制元素的文本内容
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `text`：要显示的文本内容（支持中文，内部会自动将UTF-8转换为UTF-16BE格式）

- **`clearBottomText(uint16_t screenId, uint16_t controlId)`**：清除底部文本
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素 

### 图标显示

- **`showIcon(uint16_t screenId, uint16_t controlId, uint8_t iconImageId)`**：显示指定图标的特定帧
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `iconImageId`：图标帧ID

- **`showBatteryLevel(uint16_t screenId, uint16_t controlId, int batteryLevel)`**：显示电池电量
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `batteryLevel`：电池电量百分比（0-100），内部会自动映射到1-4格显示

- **`showVolume(uint16_t screenId, uint16_t controlId, float volumeState)`**：显示音量状态
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `volumeState`：音量状态（0.0-1.0），内部会自动映射到1-4级显示

- **`showSignalLevel(uint16_t screenId, uint16_t controlId, int signalLevel)`**：显示信号强度
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `signalLevel`：信号强度等级（0-4）

- **`ChargingLogo(uint16_t screenId, uint16_t controlId, uint8_t iconImageId)`**：显示充电状态图标
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `iconImageId`：图标帧ID

- **`AlarmLogo(uint16_t screenId, uint16_t controlId, uint8_t iconImageId)`**：显示闹钟图标
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `iconImageId`：图标帧ID

### 动画控制

- **`playAnimation(uint16_t screenId, uint16_t controlId, bool isPlay)`**：播放或停止动画
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `isPlay`：`true` 播放动画，`false` 停止动画

- **`AnimationVisibility(uint16_t screenId, uint16_t controlId, bool hide)`**：显示或隐藏动画控制元素
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `controlId`：控制ID，用于定位指定界面中的具体控制元素
  - `hide`：`true` 显示动画，`false` 隐藏动画

### 情绪渲染

- **`renderEmotionByName(uint16_t screenId, const char* prevEmoName, const char* newEmoName)`**：根据情绪名称渲染指定的情绪动画
  - `screenId`：屏幕ID，用于定位控制元素所在的界面
  - `prevEmoName`：之前的情绪状态名称
  - `newEmoName`：要刷新的新情绪状态名称

### 其他功能

- **`set_pure_mode()`**：设置简单模式，隐藏部分UI元素（电池、音量、信号图标等）

## 常量定义

### 屏幕ID
- `SCREEN_WIFI = 0`：WIFI设置屏幕，用于定位WIFI相关界面
- `SCREEN_MAIN = 1`：主屏幕，用于定位主界面
- `SCREEN_NETWORK = 2`：网络设置屏幕，用于定位网络相关界面

### 控制ID
- `CONTROL_BATTERY_LEVEL = 1`：电池电量控件ID
- `CONTROL_SIGNAL_LEVEL = 2`：信号强度控件ID
- `CONTROL_VOLUME = 3`：音量控件ID
- `CONTROL_ALARM_LOGO = 4`：闹钟logo控件ID
- `CONTROL_CHARGING = 5`：充电状态控件ID
- `CONTROL_STATUS_TEXT = 6`：状态文本控件ID
- `CONTROL_TALK_TEXT = 7`：讲话中文字控件ID
- `CONTROL_LISTEN_ANIMATION = 8`：聆听动画控件ID
- `CONTROL_SLEEP_ANIMATION = 9`：睡眠动画控件ID
- `CONTROL_HAPPY_ANIMATION = 10`：开心动画控件ID
- `CONTROL_ING_ANIMATION = 11`：进行中动画控件ID
- `CONTROL_ACCIDENT_ANIMATION = 12`：意外动画控件ID
- `CONTROL_MUSIC_ANIMATION = 13`：音乐动画控件ID
- `CONTROL_ANGRY_ANIMATION = 14`：生气动画控件ID
- `CONTROL_NO_ANIMATION = 15`：拒绝动画控件ID
- `CONTROL_SAD_ANIMATION = 16`：悲伤动画控件ID
- `CONTROL_NETWORK_TEXT = 2`：网络连接状态文本控件ID

## 情绪映射

您可以通过 `EmotionControlPair` 结构体定义情绪名称与控制ID之间的映射关系：

```cpp
typedef struct {
  const char* emotionName;  // 情绪名称
  uint16_t controlId;       // 对应的控制ID
} EmotionControlPair;
```

在初始化 `SerialScreen` 对象时，可以传入自定义的情绪映射表和映射表大小。

## 示例代码

### 情绪切换示例

```cpp
// 初始化情绪状态
String prevEmotion = "休息中";

void changeEmotion(String newEmotion) {
  // 渲染新情绪并更新之前的情绪
  // 通过SCREEN_MAIN屏幕ID定位到主界面
  serialScreen.renderEmotionByName(SCREEN_MAIN, prevEmotion.c_str(), newEmotion.c_str());
  prevEmotion = newEmotion;
}

void loop() {
  // 切换到聆听状态
  changeEmotion("聆听中");
  delay(3000);
  
  // 切换到对话状态
  changeEmotion("对话中");
  delay(3000);
  
  // 更新对话文本
  // 通过SCREEN_MAIN屏幕ID定位到主界面，通过CONTROL_TALK_TEXT控制ID定位到对话文本控件
  serialScreen.updateText(SCREEN_MAIN, CONTROL_TALK_TEXT, "你好，我是智能助手！");
  delay(3000);
  
  // 切换到休息状态
  changeEmotion("休息中");
  delay(3000);
}
```

### 显示状态信息

```cpp
void updateStatusInfo(int batteryLevel, float volume, int signalStrength) {
  // 更新电池电量显示
  // 通过SCREEN_MAIN屏幕ID定位到主界面，通过CONTROL_BATTERY_LEVEL控制ID定位到电池电量控件
  serialScreen.showBatteryLevel(SCREEN_MAIN, CONTROL_BATTERY_LEVEL, batteryLevel);
  
  // 更新音量显示
  // 通过SCREEN_MAIN屏幕ID定位到主界面，通过CONTROL_VOLUME控制ID定位到音量控件
  serialScreen.showVolume(SCREEN_MAIN, CONTROL_VOLUME, volume);
  
  // 更新信号强度显示
  // 通过SCREEN_MAIN屏幕ID定位到主界面，通过CONTROL_SIGNAL_LEVEL控制ID定位到信号强度控件
  serialScreen.showSignalLevel(SCREEN_MAIN, CONTROL_SIGNAL_LEVEL, signalStrength);
}
```

### 控制元素操作示例

```cpp
void controlOperations() {
  // 动画控制元素操作示例
  // 显示对话进行中动画
  serialScreen.AnimationVisibility(SCREEN_MAIN, CONTROL_ING_ANIMATION, false);  // 显示动画控制元素
  serialScreen.playAnimation(SCREEN_MAIN, CONTROL_ING_ANIMATION, true);         // 播放动画
  delay(2000);
  
  // 隐藏并停止动画
  serialScreen.playAnimation(SCREEN_MAIN, CONTROL_ING_ANIMATION, false);        // 停止动画
  serialScreen.AnimationVisibility(SCREEN_MAIN, CONTROL_ING_ANIMATION, true);  // 隐藏动画控制元素
  
  // 图标控制元素操作示例
  // 显示充电状态图标
  serialScreen.showIcon(SCREEN_MAIN, CONTROL_CHARGING, 1);  // 显示充电图标
  
  // 文本控制元素操作示例
  // 更新状态文本
  serialScreen.updateText(SCREEN_MAIN, CONTROL_STATUS_TEXT, "系统正常运行中");
}
```

## 调试信息

库内置了详细的串口调试信息，使用标准的 `Serial` 端口输出。在使用过程中，可以通过串口监视器查看通信详情和命令执行状态。

## 注意事项

1. 使用库之前，请确保串口通信已初始化
2. 情绪名称必须与情绪映射表中定义的完全匹配
3. 启用简单模式后，部分UI元素将被隐藏
4. 更新文本时，库会自动将UTF-8格式的文本转换为UTF-16BE格式，以支持中文显示
5. 文本内容长度建议不超过200字符，以确保正确传输