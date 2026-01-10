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
 * 蓝牙管理模块实现
 *
 * 负责蓝牙A2DP连接管理和状态回调
 */

#include "bluetooth_manager.h"
#include "config_manager.h"
#include "../userconfig.h"

// 蓝牙A2DP Sink对象
static BluetoothA2DPSink a2dp_sink;

// 连接和播放状态
static bool isConnected = false;
static bool isPlaying = false;

// 当前音量值 (0-127, A2DP标准范围)
static uint8_t currentVolume = 100;  // 默认音量约78%

// AVRC 元数据信息
static String currentTitle = "";
static String currentArtist = "";
static String currentAlbum = "";

// 元数据更新回调函数指针
static void (*metadata_callback)(const char* title, const char* artist, const char* album) = nullptr;

// 曲目切换回调函数指针
static void (*track_change_callback)(bool isNext) = nullptr;

// 音量变化回调函数指针
static void (*volume_change_callback)(uint8_t volume) = nullptr;

/**
 * 初始化蓝牙A2DP接收器
 */
void initBluetooth(const char* deviceName) {
    // 配置I2S引脚（在启动A2DP之前）
  static const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = I2S_DMA_BUF_COUNT,
    .dma_buf_len = I2S_DMA_BUF_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  static const i2s_pin_config_t pin_config = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_LRCK_PIN,
    .data_out_num = I2S_DIN_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // 设置I2S配置
  a2dp_sink.set_i2s_config(i2s_config);
  a2dp_sink.set_pin_config(pin_config);

  // 启用自动重连功能
  a2dp_sink.set_auto_reconnect(BT_AUTO_RECONNECT);

  // 设置连接状态回调
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_on_audio_state_changed(audio_state_changed);

  // 设置AVRC元数据回调
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);

  // 启动A2DP蓝牙接收器（会自动初始化I2S）
  a2dp_sink.start(deviceName);
}
/**
 * 获取蓝牙A2DP Sink对象指针
 */
BluetoothA2DPSink* getA2DPSink() {
  return &a2dp_sink;
}
/**
 * 蓝牙连接状态回调函数
 */
void connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  isConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
  if (isConnected) {
    saveBluetoothConfig();
  }
}
/**
 * 音频播放状态回调函数
 */
void audio_state_changed(esp_a2d_audio_state_t state, void *ptr) {
  // 更新播放状态
  isPlaying = (state == ESP_A2D_AUDIO_STATE_STARTED);
}

/**
 * AVRC元数据回调函数
 */
void avrc_metadata_callback(uint8_t id, const uint8_t *text) {
  // 根据属性ID更新对应的元数据
  // ESP_AVRC_MD_ATTR_TITLE = 0x01
  // ESP_AVRC_MD_ATTR_ARTIST = 0x02
  // ESP_AVRC_MD_ATTR_ALBUM = 0x03

  switch (id) {
    case 0x01: // 标题
      currentTitle = String((char*)text);
      break;
    case 0x02: // 艺术家
      currentArtist = String((char*)text);
      break;
    case 0x03: // 专辑
      currentAlbum = String((char*)text);
      break;
  }

  // 如果设置了回调函数，则调用
  if (metadata_callback != nullptr) {
    metadata_callback(currentTitle.c_str(), currentArtist.c_str(), currentAlbum.c_str());
  }
}

/**
 * 获取蓝牙连接状态
 */
bool isBluetoothConnected() {
  return isConnected;
}

/**
 * 获取音频播放状态
 */
bool isAudioPlaying() {
  return isPlaying;
}

/**
 * 恢复出厂设置（清除所有配对设备）
 */
void factoryReset() {
  Serial.println("开始恢复出厂设置...");

  // 停止A2DP服务
  a2dp_sink.end();

  // 获取已配对设备数量
  int bond_device_num = esp_bt_gap_get_bond_device_num();
  if (bond_device_num > 0) {
    // 分配内存存储设备地址列表
    esp_bd_addr_t *bond_device_list = (esp_bd_addr_t *)malloc(sizeof(esp_bd_addr_t) * bond_device_num);
    if (bond_device_list) {
      if (esp_bt_gap_get_bond_device_list(&bond_device_num, bond_device_list) == ESP_OK) {
        for (int i = 0; i < bond_device_num; i++) {
          esp_bt_gap_remove_bond_device(bond_device_list[i]);
        }
      }
      free(bond_device_list);
    }
  }

  // 清除Preferences配置
  clearBluetoothConfig();
  delay(1000);
  ESP.restart();
}

/**
 * 播放/暂停切换
 */
void togglePlayPause() {
  if (!isConnected) return;
  if (isPlaying) {
    a2dp_sink.pause();
    isPlaying = false;
  } else {
    a2dp_sink.play();
    isPlaying = true;
  }
}

/**
 * 播放
 */
void playMusic() {
  if (isConnected) {
    a2dp_sink.play();
    isPlaying = true;
  }
}

/**
 * 暂停
 */
void pauseMusic() {
  if (isConnected) {
    a2dp_sink.pause();
    isPlaying = false;
  }
}

/**
 * 下一曲
 */
void nextTrack() {
  if (isConnected) {
    a2dp_sink.next();

    // 触发曲目切换回调
    if (track_change_callback != nullptr) {
      track_change_callback(true);
    }
  }
}

/**
 * 上一曲
 */
void previousTrack() {
  if (isConnected) {
    a2dp_sink.previous();

    // 触发曲目切换回调
    if (track_change_callback != nullptr) {
      track_change_callback(false);
    }
  }
}

/**
 * 设置音量 (使用A2DP AVRCP协议)
 */
void setVolume(uint8_t volume) {
  if (volume > 127) volume = 127;
  currentVolume = volume;
  a2dp_sink.set_volume(volume);

  // 触发音量变化回调
  if (volume_change_callback != nullptr) {
    volume_change_callback(volume);
  }
}
/**
 * 获取当前音量
 */
uint8_t getVolume() { return currentVolume; }

/**
 * 增加音量
 */
void increaseVolume(uint8_t step) {
  uint8_t newVolume = currentVolume + step;
  if (newVolume > 127) newVolume = 127;
  setVolume(newVolume);
}

/**
 * 减少音量
 */
void decreaseVolume(uint8_t step) {
  setVolume(currentVolume < step ? 0 : currentVolume - step);
}


/**
 * 设置元数据更新回调函数
 */
void setMetadataCallback(void (*callback)(const char*, const char*, const char*)) {
  metadata_callback = callback;
}

/**
 * 设置曲目切换回调函数
 */
void setTrackChangeCallback(void (*callback)(bool isNext)) {
  track_change_callback = callback;
}

/**
 * 设置音量变化回调函数
 */
void setVolumeChangeCallback(void (*callback)(uint8_t volume)) {
  volume_change_callback = callback;
}
/**
 * 获取当前歌曲标题
 */
String getCurrentTitle() { return currentTitle; }
/**
 * 获取当前艺术家
 */
String getCurrentArtist() { return currentArtist; }
/**
 * 获取当前专辑
 */
String getCurrentAlbum() { return currentAlbum; }
