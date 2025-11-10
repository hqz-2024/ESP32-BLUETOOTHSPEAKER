/**
 * PCA9554 IO扩展芯片处理模块头文件
 * 
 * 负责PCA9554 IO变化检测和蓝牙播放控制
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#ifndef PCA9554_HANDLER_H
#define PCA9554_HANDLER_H

#include <Arduino.h>

/**
 * 初始化PCA9554模块
 * 配置I2C、中断引脚和IO扩展芯片
 * 
 * @return true=初始化成功, false=初始化失败
 */
bool initPCA9554Handler();

/**
 * 更新PCA9554状态
 * 在主循环中调用，检查IO变化并执行相应操作
 */
void updatePCA9554();

#endif // PCA9554_HANDLER_H

