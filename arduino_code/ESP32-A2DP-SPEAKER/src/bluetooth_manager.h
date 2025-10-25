/**
 * 蓝牙管理模块头文件
 * 
 * 负责蓝牙A2DP连接管理和状态回调
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <Arduino.h>
#include "BluetoothA2DPSink.h"

/**
 * 初始化蓝牙A2DP接收器
 * 
 * @param deviceName 蓝牙设备名称
 */
void initBluetooth(const char* deviceName);

/**
 * 获取蓝牙A2DP Sink对象指针
 * 
 * @return BluetoothA2DPSink对象指针
 */
BluetoothA2DPSink* getA2DPSink();

/**
 * 蓝牙连接状态回调函数
 */
void connection_state_changed(esp_a2d_connection_state_t state, void *ptr);

/**
 * 音频播放状态回调函数
 */
void audio_state_changed(esp_a2d_audio_state_t state, void *ptr);

/**
 * AVRC元数据回调函数
 */
void avrc_metadata_callback(uint8_t data1, const uint8_t *data2);

/**
 * 获取蓝牙连接状态
 * 
 * @return true=已连接, false=未连接
 */
bool isBluetoothConnected();

/**
 * 获取音频播放状态
 * 
 * @return true=播放中, false=暂停
 */
bool isAudioPlaying();

/**
 * 恢复出厂设置（清除所有配对设备）
 */
void factoryReset();

#endif // BLUETOOTH_MANAGER_H

