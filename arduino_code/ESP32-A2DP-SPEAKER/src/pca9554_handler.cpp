/**
 * PCA9554 IO扩展芯片处理模块实现
 *
 * 按钮逻辑：
 * - IO1: 长按>=1秒减少音量(持续按住每500ms减少)，短按(100ms-1秒)上一曲
 * - IO2: 按下播放/暂停（100ms防抖）
 * - IO3: 长按>=1秒增加音量(持续按住每500ms增加)，短按(100ms-1秒)下一曲
 *
 * @author ESP-AI Team
 * @date 2024
 */

#include "pca9554_handler.h"
#include "bluetooth_manager.h"
#include "userconfig.h"
#include <Wire.h>
#include <PCA9554.h>

// PCA9554 对象
static PCA9554 ioExpander(PCA9554_ADDR);

// ==================== 时间参数 ====================
#define DEBOUNCE_TIME 200           // 防抖时间（毫秒）
#define LONG_PRESS_TIME 1000        // 长按触发时间（毫秒）
#define VOLUME_REPEAT_INTERVAL 500  // 音量重复调整间隔（毫秒）
#define VOLUME_STEP 13              // 音量步进值（约10%）
#define POLL_INTERVAL 20            // 轮询间隔（毫秒）

// ==================== IO1按钮状态 ====================
static bool io1Pressed = false;
static unsigned long io1PressStartTime = 0;
static unsigned long io1LastVolumeTime = 0;
static bool io1VolumeActive = false;

// ==================== IO2按钮状态 ====================
static unsigned long io2LastPressTime = 0;

// ==================== IO3按钮状态 ====================
static bool io3Pressed = false;
static unsigned long io3PressStartTime = 0;
static unsigned long io3LastVolumeTime = 0;
static bool io3VolumeActive = false;

// 上次轮询时间
static unsigned long lastPollTime = 0;

/**
 * 初始化PCA9554模块
 */
bool initPCA9554Handler() {
  // 初始化 I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);

  // 配置 INT 引脚为输入
  pinMode(INT_PIN, INPUT_PULLUP);

  // 初始化 PCA9554
  if (!ioExpander.begin()) {
    return false;
  }

  // 配置所有 IO 为输入
  if (!ioExpander.portMode(0xFF)) {
    return false;
  }

  Serial.println("PCA9554初始化成功");
  return true;
}

/**
 * 更新PCA9554状态 - 使用轮询方式
 */
void updatePCA9554() {
  unsigned long currentTime = millis();

  // 定期轮询IO状态
  if (currentTime - lastPollTime < POLL_INTERVAL) {
    return;
  }
  lastPollTime = currentTime;

  // 读取当前IO状态
  uint8_t ioState = 0;
  if (!ioExpander.digitalReadPort(ioState)) {
    return;
  }

  // 读取各按钮状态（LOW = 按下）
  bool io1CurrentlyPressed = !((ioState >> 1) & 1);
  bool io2CurrentlyPressed = !((ioState >> 2) & 1);
  bool io3CurrentlyPressed = !((ioState >> 3) & 1);

  // ==================== 处理IO1 ====================
  if (io1CurrentlyPressed && !io1Pressed) {
    // 按钮刚按下
    io1Pressed = true;
    io1PressStartTime = currentTime;
    io1VolumeActive = false;
  }
  else if (!io1CurrentlyPressed && io1Pressed) {
    // 按钮释放
    unsigned long pressDuration = currentTime - io1PressStartTime;

    if (!io1VolumeActive && pressDuration >= DEBOUNCE_TIME && pressDuration < LONG_PRESS_TIME) {
      // 短按: 100ms <= 持续时间 < 1000ms，执行上一曲
      previousTrack();
    }
    io1Pressed = false;
    io1VolumeActive = false;
  }
  else if (io1Pressed) {
    // 按钮持续按下
    unsigned long pressDuration = currentTime - io1PressStartTime;

    if (pressDuration >= LONG_PRESS_TIME) {
      if (!io1VolumeActive) {
        // 首次触发长按
        io1VolumeActive = true;
        io1LastVolumeTime = currentTime;
        decreaseVolume(VOLUME_STEP);
      }
      else if (currentTime - io1LastVolumeTime >= VOLUME_REPEAT_INTERVAL) {
        // 持续按住，每500ms减少一次
        io1LastVolumeTime = currentTime;
        decreaseVolume(VOLUME_STEP);
      }
    }
  }

  // ==================== 处理IO2 ====================
  if (io2CurrentlyPressed) {
    if (currentTime - io2LastPressTime >= DEBOUNCE_TIME) {
      togglePlayPause();
      io2LastPressTime = currentTime;
    }
  }

  // ==================== 处理IO3 ====================
  if (io3CurrentlyPressed && !io3Pressed) {
    // 按钮刚按下
    io3Pressed = true;
    io3PressStartTime = currentTime;
    io3VolumeActive = false;
  }
  else if (!io3CurrentlyPressed && io3Pressed) {
    // 按钮释放
    unsigned long pressDuration = currentTime - io3PressStartTime;

    if (!io3VolumeActive && pressDuration >= DEBOUNCE_TIME && pressDuration < LONG_PRESS_TIME) {
      // 短按: 100ms <= 持续时间 < 1000ms，执行下一曲
      nextTrack();
    }
    io3Pressed = false;
    io3VolumeActive = false;
  }
  else if (io3Pressed) {
    // 按钮持续按下
    unsigned long pressDuration = currentTime - io3PressStartTime;

    if (pressDuration >= LONG_PRESS_TIME) {
      if (!io3VolumeActive) {
        // 首次触发长按
        io3VolumeActive = true;
        io3LastVolumeTime = currentTime;
        increaseVolume(VOLUME_STEP);
      }
      else if (currentTime - io3LastVolumeTime >= VOLUME_REPEAT_INTERVAL) {
        // 持续按住，每500ms增加一次
        io3LastVolumeTime = currentTime;
        increaseVolume(VOLUME_STEP);
      }
    }
  }
}
