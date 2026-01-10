/**
 *  * Copyright (c) 2026 Cyberware Workshop
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
 
 * I2S音频处理模块实现
 *
 * 负责I2S硬件配置和音频数据处理
 * 音量控制使用A2DP AVRCP协议，不再使用软件音量控制
 *
 */

#include "audio_i2s.h"
#include "../userconfig.h"

/**
 * 配置PCM5102 MUTE引脚
 * 注意：ESP32-A2DP库会自动初始化I2S驱动和引脚
 */
void setupI2S() {
  // 配置MUTE引脚（可选）
  pinMode(I2S_MUTE_PIN, OUTPUT);
  digitalWrite(I2S_MUTE_PIN, HIGH); // PCM5102 MUTE引脚高电平取消静音
}

void setI2Smute(bool mute) {
  pinMode(I2S_MUTE_PIN, OUTPUT);
  digitalWrite(I2S_MUTE_PIN, !mute); // PCM5102 MUTE引脚高电平取消静音
}

/**
 * 音频数据流处理回调函数
 * 直接将音频数据输出到I2S，不进行软件音量处理
 * 音量控制由A2DP AVRCP协议在源端完成
 */
void read_data_stream(const uint8_t *data, uint32_t length) {
  if (length > 0) {
    size_t bytes_written;
    i2s_write(I2S_NUM_0, data, length, &bytes_written, portMAX_DELAY);
  }
}
