/**
 * 按钮处理模块头文件
 * 
 * 负责按钮事件检测和处理
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

/**
 * 初始化按钮处理模块
 */
void initButtonHandler();

/**
 * 更新按钮状态
 * 应在主循环中定期调用
 */
void updateButton();

/**
 * 检查多击超时
 * 应在主循环中定期调用
 */
void checkMultiClickTimeout();

#endif // BUTTON_HANDLER_H

