/**
 * 音量控制模块实现
 *
 * 音量控制使用A2DP AVRCP协议
 * 此模块提供向后兼容的接口
 *
 * @author ESP-AI Team
 * @date 2024
 */

#include "volume_control.h"
#include "bluetooth_manager.h"

/**
 * 初始化音量控制模块
 * (当前使用A2DP音量控制，无需特殊初始化)
 */
void initVolumeControl() {
  Serial.println("音量控制模块已初始化 (使用A2DP AVRCP协议)");
}

/**
 * 更新音量
 * (当前使用A2DP音量控制，由蓝牙模块自动处理)
 */
void updateVolume() {
  // A2DP音量控制由蓝牙模块处理，此函数保留以兼容旧代码
}

/**
 * 获取当前音量值
 * 转换A2DP音量(0-127)为百分比(0.0-1.0)
 */
float getCurrentVolume() {
  return (float)getVolume() / 127.0f;
}

