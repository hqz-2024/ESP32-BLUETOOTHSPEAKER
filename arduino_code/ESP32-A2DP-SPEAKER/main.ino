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
 * @author ESP-AI Team
 * @date 2024
 */

// 包含所有功能模块

#include <Arduino.h>
#include "eeui.h"
#include <WiFi.h>
#include <map>
#include <vector>

#include "userconfig.h"
#include "src/audio_i2s.h"
#include "src/bluetooth_manager.h"
#include "src/volume_control.h"
#include "src/led_control.h"
#include "src/button_handler.h"
#include "src/config_manager.h"
#include "src/pca9554_handler.h"
#include "src/album_cover_manager.h"

TFT_eSPI tft = TFT_eSPI();
EEUI eeui;

// ==================== 元数据回调函数 ====================
/**
 * AVRC元数据更新回调
 * 当接收到新的歌曲信息时调用
 */
void onMetadataUpdate(const char* title, const char* artist, const char* album) {
  Serial.println("========== 歌曲信息更新 ==========");
  Serial.printf("标题: %s\n", title);
  Serial.printf("艺术家: %s\n", artist);
  Serial.printf("专辑: %s\n", album);
  Serial.println("=================================");

  // 更新屏幕显示
  eeui.render_song_info(title, artist);
}

// ==================== 初始化函数 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 蓝牙A2DP音箱启动中...");
  Serial.println("版本：模块化架构，易于维护和扩展");
  Serial.println("========================================");
  
  setI2Smute(true);       //先静音，避免开机爆音

  // 初始化各个功能模块
  // initVolumeControl();    // 初始化音量控制
  initLedControl();       // 初始化LED控制
  initButtonHandler();    // 初始化按钮处理

  // 初始化蓝牙A2DP（会自动配置I2S）
  initBluetooth(BT_DEVICE_NAME);

  // 设置元数据回调函数
  setMetadataCallback(onMetadataUpdate);

  setI2Smute(false);       //配置完成取消静音

  // 初始化屏幕
  tft.begin();
  tft.setRotation(4); // 设置屏幕方向 V4 开发板
  eeui.begin(&tft, nullptr , 0 , SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_PAD_LEFT, SCREEN_PAD_RIGHT);

  // 初始化专辑封面管理器
  initAlbumCoverManager();

  // 初始化UI显示
  eeui.render_volume(1);
  eeui.recharge(true);
  eeui.render_battery(100);
  eeui.render_play_icon(false);      // 初始显示播放图标（暂停状态）
  eeui.render_bluetooth_icon(false); // 初始显示蓝牙图标（未连接）
  eeui.render_song_info("等待连接...", nullptr); // 初始提示信息

  // 显示圆形旋转专辑封面（使用默认封面，初始不旋转）
  eeui.render_rotating_image(getDefaultAlbumCover(), false);


  // 设置A2DP音频数据回调
  getA2DPSink()->set_stream_reader(read_data_stream, false);

  // 初始化PCA9554 IO扩展芯片
  if (initPCA9554Handler()) {
    Serial.println("PCA9554 初始化成功");
  } else {
    Serial.println("PCA9554 初始化失败，跳过IO扩展功能");
  }

  Serial.println("========================================");
  Serial.println("PCM5102音箱已启动");
  Serial.printf("蓝牙设备名称: %s\n", BT_DEVICE_NAME);
  Serial.println("等待蓝牙连接...");
}

// ==================== 主循环 ====================
void loop() {
  // 更新按钮状态
  updateButton();
  checkMultiClickTimeout();

  // 更新音量控制
  // updateVolume();

  // 更新LED状态指示
  updateRgbLed(isBluetoothConnected(), isAudioPlaying());

  // 更新PCA9554状态
  updatePCA9554();

  // 更新屏幕UI状态
  static bool lastConnectedState = false;
  static bool lastPlayingState = false;

  bool currentConnected = isBluetoothConnected();
  bool currentPlaying = isAudioPlaying();

  // 蓝牙连接状态变化
  if (currentConnected != lastConnectedState) {
    eeui.render_bluetooth_icon(currentConnected);
    if (currentConnected) {
      Serial.println("UI: 蓝牙已连接");
    } else {
      Serial.println("UI: 蓝牙已断开");
      eeui.render_song_info("等待连接...", nullptr);
    }
    lastConnectedState = currentConnected;
  }

  // 播放状态变化
  if (currentPlaying != lastPlayingState) {
    eeui.render_play_icon(currentPlaying);
    eeui.update_rotation_state(currentPlaying); // 更新旋转状态
    Serial.printf("UI: 播放状态 - %s\n", currentPlaying ? "播放中" : "已暂停");
    lastPlayingState = currentPlaying;
  }

  // 定期打印状态信息
  static unsigned long lastStatusPrint = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
    esp_a2d_connection_state_t conn_state = getA2DPSink()->get_connection_state();
    Serial.printf("状态 - 连接: %s, 播放: %s, 音量: %.2f, 重连状态: %s\n",
                  isBluetoothConnected() ? "已连接" : "未连接",
                  isAudioPlaying() ? "播放中" : "暂停",
                  getCurrentVolume(),
                  conn_state == ESP_A2D_CONNECTION_STATE_CONNECTING ? "重连中" : "空闲");

    // 打印当前歌曲信息
    String title = getCurrentTitle();
    String artist = getCurrentArtist();
    if (title.length() > 0) {
      Serial.printf("当前歌曲: %s - %s\n", title.c_str(), artist.c_str());
    }

    lastStatusPrint = currentTime;
  }

  delay(1);
}


