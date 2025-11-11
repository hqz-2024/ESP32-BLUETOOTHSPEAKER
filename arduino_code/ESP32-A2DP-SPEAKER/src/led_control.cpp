/**
 * LED控制模块实现
 * 
 * 负责WS2812 RGB LED状态指示
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#include "led_control.h"
#include "userconfig.h"
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
  // rgbLed.setBrightness(LED_B/GHTNESS);
  rgbLed.setPixelColor(0, rgbLed.Color(LED_COLOR_BLUE));  // 启动时显示蓝色
  rgbLed.show();
  Serial.println("WS2812 RGB LED已初始化");
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

      if (ledBlinkState) {
        rgbLed.setPixelColor(0, rgbLed.Color(LED_COLOR_BLUE));  // 蓝色
        rgbLed.setBrightness(LED_BRIGHTNESS);
      } else {
        rgbLed.setPixelColor(0, rgbLed.Color(LED_COLOR_OFF));    // 熄灭
      }
      rgbLed.show();
    }
  }
  else if (connected && !playing) {
    // 状态2: 已连接但未播放 - 蓝色长亮
    if (currentTime - lastLedUpdate >= 100) {
      lastLedUpdate = currentTime;
      rgbLed.setPixelColor(0, rgbLed.Color(LED_COLOR_BLUE));    // 蓝色
      rgbLed.setBrightness(LED_BRIGHTNESS);
      rgbLed.show();
    }
  }
  else if (connected && playing) {
    // 状态3: 播放中 - 绿色呼吸灯效果
    if (currentTime - lastLedUpdate >= LED_BREATH_INTERVAL) {
      lastLedUpdate = currentTime;

      // 更新呼吸亮度
      breathBrightness += breathDirection * LED_BREATH_STEP;

      // 反转方向
      if (breathBrightness >= LED_BRIGHTNESS) {
        breathBrightness = LED_BRIGHTNESS;
        breathDirection = -1.5;
      } else if (breathBrightness <= 1) {
        breathBrightness = 1;
        breathDirection = 1;
      }
      rgbLed.setPixelColor(0, rgbLed.Color(0, breathBrightness, 0));  // 绿色渐变
      rgbLed.setBrightness(LED_BRIGHTNESS);
      rgbLed.show();
    }
  }
}

