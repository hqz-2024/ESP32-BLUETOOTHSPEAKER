/**
 * LED控制模块头文件
 * 
 * 负责WS2812 RGB LED状态指示
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

/**
 * 初始化LED控制模块
 */
void initLedControl();

/**
 * 更新LED状态显示
 * 应在主循环中定期调用
 * 
 * @param connected 蓝牙连接状态
 * @param playing 音频播放状态
 */
void updateRgbLed(bool connected, bool playing);

#endif // LED_CONTROL_H

