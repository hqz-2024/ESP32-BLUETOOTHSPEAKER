/**
 * I2S音频处理模块实现
 * 
 * 负责I2S硬件配置和音频数据处理
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#include "audio_i2s.h"
#include "userconfig.h"

// 当前音量（内部变量）
static float currentVolume = DEFAULT_VOLUME;

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
 */
void read_data_stream(const uint8_t *data, uint32_t length) {
  if (currentVolume >= 0.0 && length > 0) {
    // 创建临时缓冲区用于音量调整
    uint8_t* tempBuffer = (uint8_t*)malloc(length);
    if (tempBuffer) {
      memcpy(tempBuffer, data, length);
      int16_t* audioData = (int16_t*)tempBuffer;
      int samples = length / 2;

      // 应用音量控制（限制最大增益）
      float effectiveVolume = currentVolume * VOLUME_MAX_GAIN;
      if (effectiveVolume > VOLUME_MAX_GAIN) {
        effectiveVolume = VOLUME_MAX_GAIN;
      }

      for (int i = 0; i < samples; i++) {
        audioData[i] = (int16_t)(audioData[i] * effectiveVolume);
      }

      // 输出到I2S
      size_t bytes_written;
      i2s_write(I2S_NUM_0, tempBuffer, length, &bytes_written, portMAX_DELAY);
      free(tempBuffer);
    }
  } 
  // else if (length > 0) {
  //   // 音量为0时输出静音
  //   size_t bytes_written;
  //   i2s_write(I2S_NUM_0, data, length, &bytes_written, portMAX_DELAY);
  // }
}

/**
 * 设置当前音量
 */
void setAudioVolume(float volume) {
  if (volume < 0.2) volume = 0.0;
  if (volume > 1.0) volume = 1.0;
  currentVolume = volume;
}

/**
 * 获取当前音量
 */
float getAudioVolume() {
  return currentVolume;
}

