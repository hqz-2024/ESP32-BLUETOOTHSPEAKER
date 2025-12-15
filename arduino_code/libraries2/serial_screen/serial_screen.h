#ifndef SERIAL_SCREEN_H
#define SERIAL_SCREEN_H

#include <Arduino.h>

// 屏幕ID宏定义
#define SCREEN_WIFI 0
#define SCREEN_MAIN 1
#define SCREEN_NETWORK 2
// 控件ID宏定义
#define CONTROL_BATTERY_LEVEL 1   // 电池电量控件ID
#define CONTROL_SIGNAL_LEVEL 2      // 信号强度控件ID
#define CONTROL_VOLUME 3           // 音量控件ID
#define CONTROL_ALARM_LOGO 4      // 闹钟logo控件ID
#define CONTROL_CHARGING 5        // 充电状态控件ID
#define CONTROL_STATUS_TEXT 6     // 状态文本控件ID
#define CONTROL_TALK_TEXT 7   // 讲话中文字控件ID
#define CONTROL_LISTEN_ANIMATION 8 // listen动画控件ID
#define CONTROL_SLEEP_ANIMATION 9  // sleep动画控件ID
#define CONTROL_HAPPY_ANIMATION 10  // happy动画控件ID
#define CONTROL_ING_ANIMATION 11    // ing动画控件ID
#define CONTROL_ACCIDENT_ANIMATION 12  // accident动画控件ID
#define CONTROL_MUSIC_ANIMATION 13 // music动画控件ID
#define CONTROL_ANGRY_ANIMATION 14 // angry动画控件ID
#define CONTROL_NO_ANIMATION 15    // no动画控件ID
#define CONTROL_SAD_ANIMATION 16   // sad动画控件ID
#define CONTROL_NETWORK_TEXT 2   // 网络连接状态文本控件ID

// 情绪名称到控件ID的映射结构
typedef struct {
  const char* emotionName;
  uint16_t controlId;
} EmotionControlPair;

class SerialScreen {
private:
  HardwareSerial* _serial;
  // 简洁模式标志
  static bool is_set_pure_mode;
  // 情绪映射表
  const EmotionControlPair* emotionMap;
  // 情绪映射表大小
  size_t emotionMapSize;
  
public:
  // 构造函数
  SerialScreen(HardwareSerial* serial, const EmotionControlPair* map = nullptr, size_t size = 0);
  
  // 初始化（支持完整参数配置）
  void begin(unsigned long baudrate, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1);
  
  // 清空下方文字指令
  void clearBottomText(uint16_t screenId, uint16_t controlId);

    // 更新文本显示
  void updateText(uint16_t screenId, uint16_t controlId, const char* text);

  // 显示闹钟logo指令
  void AlarmLogo(uint16_t screenId, uint16_t controlId, uint8_t iconImageId);
  
  // 显示充电状态指令
  void ChargingLogo(uint16_t screenId, uint16_t controlId, uint8_t iconImageId);
  
  // 显示电池电量
  void showBatteryLevel(uint16_t screenId, uint16_t controlId,int batteryLevel);
  
  // 显示音量状态
  void showVolume(uint16_t screenId, uint16_t controlId,float volumeState);
  
  // 显示信号强度
  void showSignalLevel(uint16_t screenId, uint16_t controlId,int signalLevel);
  
  // 显示图标帧指令 - 用于显示指定位置的图标
  void showIcon(uint16_t screenId, uint16_t controlId, uint8_t iconImageId);
  
  // 切换画面指令
  void switchScreen(uint16_t screenId);
  
  // 显示/隐藏动画控件
  void AnimationVisibility(uint16_t screenId, uint16_t controlId, bool hide);
  
  // 播放/停止动画指令
  void playAnimation(uint16_t screenId, uint16_t controlId, bool isPlay);
  
  // 渲染指定情绪动画
  void renderEmotionByName(uint16_t screenId, const char* prevEmoName, const char* newEmoName);
  
  // 设置简洁模式
  void set_pure_mode();
};

#endif // SERIAL_SCREEN_H