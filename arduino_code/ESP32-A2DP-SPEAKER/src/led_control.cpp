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
 * LED控制模块实现
 * 
 * 负责WS2812 RGB LED状态指示
 * 
 */

#include "led_control.h"
#include "../userconfig.h"
#include <Adafruit_NeoPixel.h>

// WS2812 RGB LED对象
static Adafruit_NeoPixel rgbLed(WS2812_LED_COUNT, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// LED控制变量
static unsigned long lastLedUpdate = 0;
static int breathBrightness = 0;
static int breathDirection = 1;
static bool ledBlinkState = false;

/**
 * 初始化LED控制模块
 */
void initLedControl() {
  rgbLed.begin();
  rgbLed.setPixelColor(0, rgbLed.Color(LED_COLOR_BLUE));
  rgbLed.show();
}

/**
 * 更新LED状态显示
 */
void updateRgbLed(bool connected, bool playing) {
  unsigned long currentTime = millis();

  if (!connected) {
    // 状态1: 未连接 - 蓝色闪烁，间隔1秒
    if (currentTime - lastLedUpdate >= LED_BLINK_INTERVAL) {
      lastLedUpdate = currentTime;
      ledBlinkState = !ledBlinkState;
      rgbLed.setPixelColor(0, ledBlinkState ? rgbLed.Color(LED_COLOR_BLUE) : rgbLed.Color(LED_COLOR_OFF));
      rgbLed.setBrightness(LED_BRIGHTNESS);
      rgbLed.show();
    }
  } else if (!playing) {
    if (currentTime - lastLedUpdate >= 100) {
      lastLedUpdate = currentTime;
      rgbLed.setPixelColor(0, rgbLed.Color(LED_COLOR_BLUE));
      rgbLed.setBrightness(LED_BRIGHTNESS);
      rgbLed.show();
    }
  } else {
    if (currentTime - lastLedUpdate >= LED_BREATH_INTERVAL) {
      lastLedUpdate = currentTime;

      // 更新呼吸亮度
      breathBrightness += breathDirection * LED_BREATH_STEP;

      // 反转方向
      if (breathBrightness >= LED_BRIGHTNESS) {
        breathBrightness = LED_BRIGHTNESS;
        breathDirection = -1;
      } else if (breathBrightness <= 1) {
        breathBrightness = 1;
        breathDirection = 1;
      }
      rgbLed.setPixelColor(0, rgbLed.Color(0, breathBrightness, 0));
      rgbLed.setBrightness(LED_BRIGHTNESS);
      rgbLed.show();
    }
  }
}
