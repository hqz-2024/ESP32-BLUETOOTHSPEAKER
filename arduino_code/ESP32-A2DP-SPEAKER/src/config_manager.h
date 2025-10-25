/**
 * 配置管理模块头文件
 * 
 * 负责蓝牙配对信息的保存和加载
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

/**
 * 保存蓝牙配置信息
 */
void saveBluetoothConfig();

/**
 * 加载蓝牙配置信息
 * 
 * @return true=已配对, false=未配对
 */
bool loadBluetoothConfig();

/**
 * 清除蓝牙配置信息
 */
void clearBluetoothConfig();

#endif // CONFIG_MANAGER_H

