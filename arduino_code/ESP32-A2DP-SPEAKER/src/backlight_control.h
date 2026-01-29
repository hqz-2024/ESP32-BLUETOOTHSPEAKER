/**
 * Copyright (c) 2026 义体工坊团队
 * MIT License
 *
 * 屏幕背光控制模块
 */

#ifndef BACKLIGHT_CONTROL_H
#define BACKLIGHT_CONTROL_H

#include <Arduino.h>

// 初始化背光控制（默认高电平关闭屏幕）
void initBacklight();

// 开启背光（低电平）
void setBacklightOn();

// 关闭背光（高电平）
void setBacklightOff();

// 重置背光计时器
void resetBacklightTimer();

// 更新背光状态（检查超时）
void updateBacklight();

#endif

