/**
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
 * 
 * PCA9554 IO扩展芯片处理模块实现
 *
 * 按钮逻辑：
 * - IO1: 长按>=1秒减少音量(持续按住每500ms减少)，短按(100ms-1秒)上一曲
 * - IO2: 按下播放/暂停（100ms防抖）
 * - IO3: 长按>=1秒增加音量(持续按住每500ms增加)，短按(100ms-1秒)下一曲
 *
 */

#include "pca9554_handler.h"
#include "bluetooth_manager.h"
#include "../userconfig.h"
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
  // 配置 INT 引脚为输入
  pinMode(INT_PIN, INPUT_PULLUP);
  if (!ioExpander.begin()) return false;
  if (!ioExpander.portMode(0xFF)) return false;
  return true;
}

/**
 * 更新PCA9554状态 - 使用轮询方式
 * @return true=有按钮操作, false=无操作
 */
bool updatePCA9554() {
  unsigned long currentTime = millis();
  if (currentTime - lastPollTime < POLL_INTERVAL) return false;
  lastPollTime = currentTime;

  // 读取当前IO状态
  uint8_t ioState = 0;
  if (!ioExpander.digitalReadPort(ioState)) return false;

  bool hasAction = false;

  // 读取各按钮状态（LOW = 按下）
  bool io1CurrentlyPressed = !((ioState >> 1) & 1);
  bool io2CurrentlyPressed = !((ioState >> 2) & 1);
  bool io3CurrentlyPressed = !((ioState >> 3) & 1);

  // ==================== 处理IO1 ====================
  if (io1CurrentlyPressed && !io1Pressed) {
    io1Pressed = true;
    io1PressStartTime = currentTime;
    io1VolumeActive = false;
    hasAction = true;
  } else if (!io1CurrentlyPressed && io1Pressed) {
    unsigned long pressDuration = currentTime - io1PressStartTime;
    if (!io1VolumeActive && pressDuration >= DEBOUNCE_TIME && pressDuration < LONG_PRESS_TIME) {
      previousTrack();
    }
    io1Pressed = false;
    io1VolumeActive = false;
    hasAction = true;
  } else if (io1Pressed) {
    unsigned long pressDuration = currentTime - io1PressStartTime;
    if (pressDuration >= LONG_PRESS_TIME) {
      if (!io1VolumeActive) {
        io1VolumeActive = true;
        io1LastVolumeTime = currentTime;
        decreaseVolume(VOLUME_STEP);
        hasAction = true;
      } else if (currentTime - io1LastVolumeTime >= VOLUME_REPEAT_INTERVAL) {
        io1LastVolumeTime = currentTime;
        decreaseVolume(VOLUME_STEP);
        hasAction = true;
      }
    }
  }

  if (io2CurrentlyPressed && currentTime - io2LastPressTime >= DEBOUNCE_TIME) {
    togglePlayPause();
    io2LastPressTime = currentTime;
    hasAction = true;
  }

  // ==================== 处理IO3 ====================
  if (io3CurrentlyPressed && !io3Pressed) {
    io3Pressed = true;
    io3PressStartTime = currentTime;
    io3VolumeActive = false;
    hasAction = true;
  } else if (!io3CurrentlyPressed && io3Pressed) {
    unsigned long pressDuration = currentTime - io3PressStartTime;
    if (!io3VolumeActive && pressDuration >= DEBOUNCE_TIME && pressDuration < LONG_PRESS_TIME) {
      nextTrack();
    }
    io3Pressed = false;
    io3VolumeActive = false;
    hasAction = true;
  } else if (io3Pressed) {
    unsigned long pressDuration = currentTime - io3PressStartTime;
    if (pressDuration >= LONG_PRESS_TIME) {
      if (!io3VolumeActive) {
        io3VolumeActive = true;
        io3LastVolumeTime = currentTime;
        increaseVolume(VOLUME_STEP);
        hasAction = true;
      } else if (currentTime - io3LastVolumeTime >= VOLUME_REPEAT_INTERVAL) {
        io3LastVolumeTime = currentTime;
        increaseVolume(VOLUME_STEP);
        hasAction = true;
      }
    }
  }
  return hasAction;
}
