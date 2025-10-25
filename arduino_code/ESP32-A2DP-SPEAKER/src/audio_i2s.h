/**
 * I2S音频处理模块头文件
 * 
 * 负责I2S硬件配置和音频数据处理
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef AUDIO_I2S_H
#define AUDIO_I2S_H

#include <Arduino.h>
#include "driver/i2s.h"

/**
 * 初始化I2S硬件
 * 配置I2S引脚和参数，适配PCM5102 DAC芯片
 */
void setupI2S();

/**
 * 音频数据流处理回调函数
 * 用于A2DP音频数据接收和音量控制
 * 
 * @param data 音频数据指针
 * @param length 数据长度（字节）
 */
void read_data_stream(const uint8_t *data, uint32_t length);

/**
 * 设置当前音量
 * 
 * @param volume 音量值 (0.0 - 1.0)
 */
void setAudioVolume(float volume);

/**
 * 获取当前音量
 * 
 * @return 当前音量值 (0.0 - 1.0)
 */
float getAudioVolume();

#endif // AUDIO_I2S_H

