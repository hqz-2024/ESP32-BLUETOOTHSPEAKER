/**
 * * Copyright (c) 2026 Cyberware Workshop
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Commercial use of this software requires prior written authorization from the Licensor.
 * 请注意：将 Cyberware Workshop 代码用于商业用途需要事先获得许可方的授权。
 * 删除与修改版权属于侵权行为，请尊重作者版权，避免产生不必要的纠纷。
 *
 * @author Cyberware Workshop Team
 * @date 2026
 * 
 * I2S音频处理模块头文件
 *
 * 负责I2S硬件配置和音频数据处理
 * 音量控制使用A2DP AVRCP协议
 *
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
 * 直接将A2DP音频数据输出到I2S
 *
 * @param data 音频数据指针
 * @param length 数据长度（字节）
 */
void read_data_stream(const uint8_t *data, uint32_t length);

/**
 * 设置I2S静音状态
 *
 * @param mute true=静音, false=取消静音
 */
void setI2Smute(bool mute);

#endif // AUDIO_I2S_H
