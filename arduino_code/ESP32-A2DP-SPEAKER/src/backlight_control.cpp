/**
 * Copyright (c) 2026 义体工坊团队
 * MIT License
 *
 * 屏幕背光控制模块实现
 */

#include "backlight_control.h"
#include "../userconfig.h"

static unsigned long lastActivityTime = 0;
static bool backlightOn = false;

void initBacklight() {
  pinMode(SCREEN_BL_PIN, OUTPUT);
  digitalWrite(SCREEN_BL_PIN, HIGH);
  backlightOn = false;
}

void setBacklightOn() {
  digitalWrite(SCREEN_BL_PIN, LOW);
  backlightOn = true;
  lastActivityTime = millis();
}

void setBacklightOff() {
  digitalWrite(SCREEN_BL_PIN, HIGH);
  backlightOn = false;
}

void resetBacklightTimer() {
  lastActivityTime = millis();
  if (!backlightOn) {
    setBacklightOn();
  }
}

void updateBacklight() {
  if (backlightOn && (millis() - lastActivityTime >= SCREEN_TIMEOUT_MS)) {
    setBacklightOff();
  }
}

