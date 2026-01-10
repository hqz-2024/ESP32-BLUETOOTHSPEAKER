/**
 * ESP32 经典蓝牙A2DP音箱程序 - 模块化版本
 *
 * 重要说明：此程序仅适用于ESP32经典版本，不支持ESP32S3！
 *
 * 功能特性：
 * 1. 蓝牙A2DP接收器，设备名称：ESP-AI-SPEAKER
 * 2. I2S音频输出到PCM5102芯片
 * 3. IO0按钮连按5下恢复出厂设置
 * 4. IO34 ADC音量控制
 * 5. 自动重连已配对设备
 * 6. 低延时高音质音频传输
 * 7. WS2812 RGB LED状态指示
 *    - 未连接：蓝色闪烁（1秒间隔）
 *    - 已连接未播放：蓝色长亮
 *    - 播放中：绿色呼吸灯效果
 *
 * 硬件连接：
 * PCM5102 DAC模块：
 *   - BCK  -> GPIO33 (I2S位时钟)
 *   - LRCK -> GPIO26 (I2S左右声道时钟)
 *   - DIN  -> GPIO25 (I2S数据输入)
 *   - GND  -> GND
 *   - VIN  -> 3.3V
 *   - SCK  -> GND (不使用主时钟)
 *   - MUTE -> GPIO27 (静音控制，可选)
 *
 * 其他引脚：
 *   - IO0  -> BOOT按钮（连按5下恢复出厂设置）
 *   - IO34 -> 音量控制ADC输入（可选）
 *   - IO12 -> WS2812 RGB LED数据引脚
 *
 * 使用方法：
 * 1. 首次开机进入蓝牙广播模式（蓝色闪烁）
 * 2. 手机搜索"ESP-AI-SPEAKER"并连接
 * 3. 连接成功后LED变为蓝色长亮
 * 4. 播放音乐时LED显示绿色呼吸灯效果
 * 5. 支持所有标准A2DP设备
 *
 * 模块说明：
 * - src/userconfig.h      - 硬件配置和引脚定义
 * - src/audio_i2s.*       - I2S音频处理模块
 * - src/bluetooth_manager.* - 蓝牙管理模块
 * - src/volume_control.*  - 音量控制模块
 * - src/led_control.*     - LED控制模块
 * - src/button_handler.*  - 按钮处理模块
 * - src/config_manager.*  - 配置管理模块
 * 
 * /**
 * Copyright (c) 2026 Cyberware Workshop
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
 */

// 包含所有功能模块

#include <Arduino.h>
#include <Battery.h>
#include "eeui.h"
#include "userconfig.h"
#include "src/audio_i2s.h"
#include "src/bluetooth_manager.h"
#include "src/volume_control.h"
#include "src/led_control.h"
#include "src/button_handler.h"
#include "src/config_manager.h"
#include "src/pca9554_handler.h"
#include "src/album_cover_manager.h"
#include "src/qmi8658_handler.h"

Battery battery(3000, 4200, BATTERY_PIN, 12);
TFT_eSPI tft = TFT_eSPI();
EEUI eeui;

void onMetadataUpdate(const char* title, const char* artist, const char* album) {
  eeui.render_song_info(title, artist);
}

void onTrackChange(bool isNext) {
  const lv_img_dsc_t* newCover = isNext ? nextAlbumCover() : previousAlbumCover();
  eeui.render_rotating_image(newCover, isAudioPlaying());
}

void onVolumeChange(uint8_t volume) {
  eeui.render_volume((float)volume / 127.0f);
}

void updateBattery() {
  static unsigned long lastBatteryUpdate = 0;
  if (millis() - lastBatteryUpdate > 3000) {
    eeui.render_battery(battery.level());
    lastBatteryUpdate = millis();
  }
}

void setup() {
  Serial.begin(115200);
  setI2Smute(true);

  initLedControl();
  initButtonHandler();
  initPCA9554Handler();
  initBluetooth(BT_DEVICE_NAME);

  setI2Smute(false);

  battery.begin(3300, 4.0, &asigmoidal);
  tft.begin();
  tft.setRotation(4);
  eeui.begin(&tft, nullptr, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_PAD_LEFT, SCREEN_PAD_RIGHT);

  initAlbumCoverManager();
  initQMI8658Handler();
  setQMI8658EEUIInstance(&eeui);

  setMetadataCallback(onMetadataUpdate);
  setTrackChangeCallback(onTrackChange);
  setVolumeChangeCallback(onVolumeChange);

  eeui.render_volume(1);
  eeui.recharge(battery.getischarge());
  eeui.render_battery(battery.level());
  eeui.render_play_icon(false);
  eeui.render_bluetooth_icon(false);
  eeui.render_song_info("等待连接...", nullptr);
  eeui.render_rotating_image(getDefaultAlbumCover(), false);

  getA2DPSink()->set_stream_reader(read_data_stream, false);
}

void loop() {
  updateButton();
  checkMultiClickTimeout();
  updateBattery();
  updateQMI8658();
  updateRgbLed(isBluetoothConnected(), isAudioPlaying());
  updatePCA9554();

  static bool lastConnectedState = false;
  static bool lastPlayingState = false;

  bool currentConnected = isBluetoothConnected();
  bool currentPlaying = isAudioPlaying();

  if (currentConnected != lastConnectedState) {
    eeui.render_bluetooth_icon(currentConnected);
    if (!currentConnected) {
      eeui.render_song_info("等待连接...", nullptr);
    }
    lastConnectedState = currentConnected;
  }

  if (currentPlaying != lastPlayingState) {
    eeui.render_play_icon(currentPlaying);
    eeui.update_rotation_state(currentPlaying);
    lastPlayingState = currentPlaying;
  }

  delay(1);
}
