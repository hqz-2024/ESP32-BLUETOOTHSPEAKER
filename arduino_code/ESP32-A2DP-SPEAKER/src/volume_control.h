/**
 * 音量控制模块头文件
 *
 * 音量控制使用A2DP AVRCP协议
 * 此模块提供向后兼容的接口
 *
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef VOLUME_CONTROL_H
#define VOLUME_CONTROL_H

#include <Arduino.h>

/**
 * 初始化音量控制模块
 * (当前使用A2DP音量控制，无需特殊初始化)
 */
void initVolumeControl();

/**
 * 更新音量
 * (当前使用A2DP音量控制，由蓝牙模块自动处理)
 */
void updateVolume();

/**
 * 获取当前音量值
 *
 * @return 音量值 (0.0 - 1.0)
 */
float getCurrentVolume();

#endif // VOLUME_CONTROL_H

