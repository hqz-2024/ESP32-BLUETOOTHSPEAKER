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
 * 按钮处理模块实现
 * 
 * 负责按钮事件检测和处理
 * 
 */

#include "button_handler.h"
#include "bluetooth_manager.h"
#include "backlight_control.h"
#include "../userconfig.h"
#include "OneButton.h"

// 按钮对象
static OneButton bootButton(BOOT_BUTTON_PIN, true);

// 按钮点击计数
static int buttonClickCount = 0;
static unsigned long lastClickTime = 0;
static unsigned long lastValidClickTime = 0;  // 上次有效点击时间

/**
 * 按钮点击事件处理函数
 * 增加了防误触发机制：
 * 1. 增加去抖动时间
 * 2. 检查两次点击之间的最小间隔
 * 3. 过滤掉过快的连续点击（可能是干扰）
 */
static void handleButtonClick() {
  unsigned long currentTime = millis();
  unsigned long timeSinceLastValid = currentTime - lastValidClickTime;

  // 防止过快的连续点击（可能是音频干扰导致的误触发）
  if (lastValidClickTime > 0 && timeSinceLastValid < BUTTON_IDLE_TICKS) {
    return;
  }

  // 检查多击超时
  if (currentTime - lastClickTime > MULTI_CLICK_TIMEOUT) {
    buttonClickCount = 0;
  }

  buttonClickCount++;
  lastClickTime = currentTime;
  lastValidClickTime = currentTime;
  resetBacklightTimer();

  if (buttonClickCount == FACTORY_RESET_CLICKS) {
    factoryReset();
    buttonClickCount = 0;
  }
}

/**
 * 初始化按钮处理模块
 * 使用更严格的防抖参数防止音频干扰导致的误触发
 */
void initButtonHandler() {
  bootButton.attachClick(handleButtonClick);
  bootButton.setClickTicks(BUTTON_CLICK_TICKS);
  bootButton.setPressTicks(BUTTON_PRESS_TICKS);
  bootButton.setDebounceTicks(BUTTON_DEBOUNCE_TICKS);// 100ms防抖
  // 注意：空闲时间检查在handleButtonClick()中手动实现
}
/**
 * 更新按钮状态
 */
void updateButton() {
  bootButton.tick();
}

/**
 * 检查多击超时
 */
void checkMultiClickTimeout() {
  if (buttonClickCount > 0 && (millis() - lastClickTime > MULTI_CLICK_TIMEOUT)) {
    buttonClickCount = 0;
  }
}
