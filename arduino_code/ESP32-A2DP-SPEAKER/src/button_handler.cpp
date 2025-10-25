/**
 * 按钮处理模块实现
 * 
 * 负责按钮事件检测和处理
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#include "button_handler.h"
#include "bluetooth_manager.h"
#include "userconfig.h"
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
    Serial.printf("⚠️ 按钮点击过快，忽略 (间隔: %lu ms < %d ms)\n",
                  timeSinceLastValid, BUTTON_IDLE_TICKS);
    return;
  }

  // 检查多击超时
  if (currentTime - lastClickTime > MULTI_CLICK_TIMEOUT) {
    if (buttonClickCount > 0) {
      Serial.printf("多击超时，重置计数 (上次计数: %d)\n", buttonClickCount);
    }
    buttonClickCount = 0;
  }

  buttonClickCount++;
  lastClickTime = currentTime;

  Serial.printf("✓ 按钮点击次数: %d/%d (间隔: %lu ms)\n",
                buttonClickCount, FACTORY_RESET_CLICKS, timeSinceLastValid);

  lastValidClickTime = currentTime;

  if (buttonClickCount == FACTORY_RESET_CLICKS) {
    Serial.printf("🔄 检测到%d次点击，执行恢复出厂设置...\n", FACTORY_RESET_CLICKS);
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
  bootButton.setDebounceTicks(BUTTON_DEBOUNCE_TICKS);  // 100ms防抖
  // 注意：空闲时间检查在handleButtonClick()中手动实现

  Serial.println("按钮处理模块已初始化");
  Serial.printf("  - 防抖时间: %d ms\n", BUTTON_DEBOUNCE_TICKS);
  Serial.printf("  - 最小点击间隔: %d ms\n", BUTTON_IDLE_TICKS);
  Serial.printf("  - 恢复出厂需要: %d 次点击\n", FACTORY_RESET_CLICKS);
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
    if (buttonClickCount < FACTORY_RESET_CLICKS) {
      Serial.printf("多击超时，点击次数: %d (需要%d次)\n", 
                    buttonClickCount, FACTORY_RESET_CLICKS);
    }
    buttonClickCount = 0;
  }
}

