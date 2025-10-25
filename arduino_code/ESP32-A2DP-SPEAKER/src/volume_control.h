/**
 * 音量控制模块头文件
 * 
 * 负责ADC音量检测和音量管理
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef VOLUME_CONTROL_H
#define VOLUME_CONTROL_H

#include <Arduino.h>

/**
 * 初始化音量控制模块
 */
void initVolumeControl();

/**
 * 更新音量（从ADC读取）
 * 应在主循环中定期调用
 */
void updateVolume();

/**
 * 获取当前音量值
 * 
 * @return 音量值 (0.0 - 1.0)
 */
float getCurrentVolume();

#endif // VOLUME_CONTROL_H

