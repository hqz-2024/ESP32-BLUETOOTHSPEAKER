/**
 * ESP32 经典蓝牙A2DP音箱程序 - 模块化版本
 *
 * 重要说明：此程序仅适用于ESP32经典版本，不支持ESP32S3！
 *
 * 功能特性：
 * 1. 蓝牙A2DP接收器，设备名称：ESP-AI-SPEAKER
 * 2. I2S音频输出到PCM5102芯片
 * 3. IO0按钮连按5下恢复出厂设置
 * 4. 自动重连已配对设备
 * 5. 低延时高音质音频传输
 * 6. WS2812 RGB LED状态指示
 * 7. TFT屏幕显示歌曲信息、专辑封面、电量、播放状态
 *
 * Copyright (c) 2026 Cyberware Workshop
 */

#include <Arduino.h>
#include <Wire.h>
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
#include "src/backlight_control.h"

#define UI_TEXT_BUF_SIZE 128

Battery battery(3000, 4200, BATTERY_PIN, 12);
TFT_eSPI tft = TFT_eSPI();
EEUI eeui;

static portMUX_TYPE uiStateMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool songInfoDirty = false;
static volatile bool trackChangeDirty = false;
static volatile bool volumeDirty = false;
static volatile bool coverDirty = false;
static volatile int pendingTrackDirection = 0;
static volatile uint8_t pendingVolume = 127;
static char pendingTitle[UI_TEXT_BUF_SIZE] = "";
static char pendingArtist[UI_TEXT_BUF_SIZE] = "";
static char displayedTitle[UI_TEXT_BUF_SIZE] = "";
static char displayedArtist[UI_TEXT_BUF_SIZE] = "";

/**
 * 复制字符串到固定缓冲区
 */
static void copyTextSafely(char *dest, const char *src, size_t size) {
  if (size == 0) return;
  if (src == nullptr) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
}

/**
 * 接收蓝牙元数据更新，只缓存数据
 */
void onMetadataUpdate(const char* title, const char* artist, const char* album) {
  (void)album;
  portENTER_CRITICAL(&uiStateMux);
  if (title != nullptr && title[0] != '\0') {
    copyTextSafely(pendingTitle, title, sizeof(pendingTitle));
  }
  if (artist != nullptr && artist[0] != '\0') {
    copyTextSafely(pendingArtist, artist, sizeof(pendingArtist));
  }
  if (pendingTitle[0] != '\0' || pendingArtist[0] != '\0') {
    songInfoDirty = true;
  }
  portEXIT_CRITICAL(&uiStateMux);
}

/**
 * 接收曲目切换事件，只记录方向
 */
void onTrackChange(bool isNext) {
  portENTER_CRITICAL(&uiStateMux);
  pendingTrackDirection = isNext ? 1 : -1;
  trackChangeDirty = true;
  portEXIT_CRITICAL(&uiStateMux);
}

/**
 * 接收音量变化事件，只缓存音量值
 */
void onVolumeChange(uint8_t volume) {
  portENTER_CRITICAL(&uiStateMux);
  pendingVolume = volume;
  volumeDirty = true;
  portEXIT_CRITICAL(&uiStateMux);
}

/**
 * 定时更新电池显示
 */
void updateBattery() {
  static unsigned long lastBatteryUpdate = 0;
  if (millis() - lastBatteryUpdate > 3000) {
    eeui.render_battery(battery.level());
    lastBatteryUpdate = millis();
  }
}

/**
 * 在主循环中统一处理待刷新的UI状态
 */
static void processPendingUiUpdates() {
  bool needSongInfoUpdate = false;
  bool needTrackChangeUpdate = false;
  bool needVolumeUpdate = false;
  bool needCoverRefresh = false;
  int localTrackDirection = 0;
  uint8_t localVolume = 0;
  char localTitle[UI_TEXT_BUF_SIZE] = "";
  char localArtist[UI_TEXT_BUF_SIZE] = "";

  portENTER_CRITICAL(&uiStateMux);
  if (songInfoDirty) {
    copyTextSafely(localTitle, pendingTitle, sizeof(localTitle));
    copyTextSafely(localArtist, pendingArtist, sizeof(localArtist));
    songInfoDirty = false;
    needSongInfoUpdate = true;
  }
  if (trackChangeDirty) {
    localTrackDirection = pendingTrackDirection;
    pendingTrackDirection = 0;
    trackChangeDirty = false;
    needTrackChangeUpdate = true;
  }
  if (volumeDirty) {
    localVolume = pendingVolume;
    volumeDirty = false;
    needVolumeUpdate = true;
  }
  if (coverDirty) {
    coverDirty = false;
    needCoverRefresh = true;
  }
  portEXIT_CRITICAL(&uiStateMux);

  if (needSongInfoUpdate) {
    copyTextSafely(displayedTitle, localTitle, sizeof(displayedTitle));
    copyTextSafely(displayedArtist, localArtist, sizeof(displayedArtist));
    eeui.render_song_info(displayedTitle[0] ? displayedTitle : "等待连接...", displayedArtist[0] ? displayedArtist : nullptr);
  }

  if (needTrackChangeUpdate) {
    const lv_img_dsc_t* newCover = localTrackDirection >= 0 ? nextAlbumCover() : previousAlbumCover();
    eeui.render_rotating_image(newCover, isAudioPlaying());
  } else if (needCoverRefresh) {
    eeui.render_rotating_image(getCurrentAlbumCover(), isAudioPlaying());
  }

  if (needVolumeUpdate) {
    eeui.render_volume((float)localVolume / 127.0f);
  }
}

void setup() {
  Serial.begin(115200);
  setI2Smute(true);
  initBacklight();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  Wire.setClock(I2C_FREQ);

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
  copyTextSafely(displayedTitle, "等待连接...", sizeof(displayedTitle));
  displayedArtist[0] = '\0';
  eeui.render_song_info(displayedTitle, nullptr);
  eeui.render_rotating_image(getDefaultAlbumCover(), false);

  getA2DPSink()->set_stream_reader(read_data_stream, false);
  setBacklightOn();
}

void loop() {
  updateButton();
  checkMultiClickTimeout();
  updateBattery();
  updateQMI8658();
  updateRgbLed(isBluetoothConnected(), isAudioPlaying());
  if (updatePCA9554()) resetBacklightTimer();
  updateBacklight();
  processPendingUiUpdates();

  static bool lastConnectedState = false;
  static bool lastPlayingState = false;

  bool currentConnected = isBluetoothConnected();
  bool currentPlaying = isAudioPlaying();

  if (currentConnected != lastConnectedState) {
    eeui.render_bluetooth_icon(currentConnected);
    if (!currentConnected) {
      portENTER_CRITICAL(&uiStateMux);
      pendingTitle[0] = '\0';
      pendingArtist[0] = '\0';
      displayedArtist[0] = '\0';
      pendingTrackDirection = 0;
      trackChangeDirty = false;
      portEXIT_CRITICAL(&uiStateMux);
      copyTextSafely(displayedTitle, "等待连接...", sizeof(displayedTitle));
      eeui.render_song_info(displayedTitle, nullptr);
      eeui.render_rotating_image(getDefaultAlbumCover(), false);
    } else {
      bool snapshotConnected = false;
      bool snapshotPlaying = false;
      uint8_t snapshotVolume = 0;
      char snapshotTitle[UI_TEXT_BUF_SIZE] = "";
      char snapshotArtist[UI_TEXT_BUF_SIZE] = "";
      char snapshotAlbum[UI_TEXT_BUF_SIZE] = "";
      getBluetoothUiSnapshot(&snapshotConnected, &snapshotPlaying, &snapshotVolume, snapshotTitle, sizeof(snapshotTitle), snapshotArtist, sizeof(snapshotArtist), snapshotAlbum, sizeof(snapshotAlbum));
      if (snapshotTitle[0] != '\0') {
        copyTextSafely(displayedTitle, snapshotTitle, sizeof(displayedTitle));
        copyTextSafely(displayedArtist, snapshotArtist, sizeof(displayedArtist));
        eeui.render_song_info(displayedTitle, displayedArtist[0] ? displayedArtist : nullptr);
      }
      eeui.render_volume((float)snapshotVolume / 127.0f);
      portENTER_CRITICAL(&uiStateMux);
      pendingVolume = snapshotVolume;
      volumeDirty = false;
      coverDirty = true;
      portEXIT_CRITICAL(&uiStateMux);
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
