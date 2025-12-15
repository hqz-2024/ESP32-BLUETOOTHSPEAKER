/**
 * I2S音频处理模块实现
 *
 * 负责I2S硬件配置和音频数据处理
 * 音量控制使用A2DP AVRCP协议，不再使用软件音量控制
 *
 * @author ESP-AI Team
 * @date 2024
 */

#include "audio_i2s.h"
#include "userconfig.h"

/**
 * 配置PCM5102 MUTE引脚
 * 注意：ESP32-A2DP库会自动初始化I2S驱动和引脚
 */
void setupI2S() {
  // 配置MUTE引脚（可选）
  pinMode(I2S_MUTE_PIN, OUTPUT);
  digitalWrite(I2S_MUTE_PIN, HIGH); // PCM5102 MUTE引脚高电平取消静音

  Serial.println("PCM5102 MUTE引脚配置完成");
  Serial.printf("  - MUTE引脚: %d (HIGH=取消静音)\n", I2S_MUTE_PIN);
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

