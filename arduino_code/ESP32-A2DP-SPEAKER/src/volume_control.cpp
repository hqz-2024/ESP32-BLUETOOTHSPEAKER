/**
 * 音量控制模块实现
 * 
 * 负责ADC音量检测和音量管理
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#include "volume_control.h"
#include "audio_i2s.h"
#include "userconfig.h"

// 音量检查时间戳
static unsigned long lastVolumeCheck = 0;

/**
 * 初始化音量控制模块
 */
void initVolumeControl() {
  // 配置ADC分辨率
  analogReadResolution(12);
  Serial.println("音量控制模块已初始化");
}

/**
 * 更新音量（从ADC读取）
 */
void updateVolume() {
  unsigned long currentTime = millis();
  
  // 检查是否到达更新间隔
  if (currentTime - lastVolumeCheck < VOLUME_CHECK_INTERVAL) {
    return;
  }
  
  lastVolumeCheck = currentTime;
  
  // 读取ADC值
  int adcValue = analogRead(VOLUME_ADC_PIN);

  // 转换为0-1范围
  float rawVolume = (float)adcValue / 4095.0;

  // 量化为指定分辨率（默认0.05的分辨率）
  // 共21个档位：0, 0.05, 0.10, 0.15, 0.20, ..., 0.95, 1.00
  float quantizedVolume = round(rawVolume * VOLUME_QUANTIZE_STEPS) / VOLUME_QUANTIZE_STEPS;

  // 限制范围在0.0-1.0之间
  if (quantizedVolume < 0.0) quantizedVolume = 0.0;
  if (quantizedVolume > 1.0) quantizedVolume = 1.0;

  // 获取当前音量
  float currentVolume = getAudioVolume();

  // 只有当量化后的值发生变化时才更新
  if (fabs(quantizedVolume - currentVolume) > 0.04) {
    setAudioVolume(quantizedVolume);
    Serial.printf("音量调整: %.2f (ADC: %d, 档位: %d/%d)\n",
                  quantizedVolume, adcValue, 
                  (int)(quantizedVolume * VOLUME_QUANTIZE_STEPS), 
                  VOLUME_QUANTIZE_STEPS);
  }
}

/**
 * 获取当前音量值
 */
float getCurrentVolume() {
  return getAudioVolume();
}

