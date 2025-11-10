/**
 * PCA9554 IO扩展芯片处理模块实现
 * 
 * 负责PCA9554 IO变化检测和蓝牙播放控制
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

// 上次的 IO 状态
static uint8_t lastIOState = 0xFF;

// 中断标志
static volatile bool interruptTriggered = false;

// 按钮防抖时间戳
static unsigned long lastIO1Change = 0;
static unsigned long lastIO2Change = 0;
static unsigned long lastIO3Change = 0;

// 防抖延迟（毫秒）
#define DEBOUNCE_DELAY 200

/**
 * 中断处理函数
 */
void IRAM_ATTR handlePCA9554Interrupt() {
  interruptTriggered = true;
}

/**
 * 处理IO变化
 */
static void handleIOChange(uint8_t currentState) {
  // 检查是否有变化
  if (currentState == lastIOState) {
    return;
  }
  
  uint8_t changed = currentState ^ lastIOState;
  unsigned long currentTime = millis();
  
  // 检查各个IO引脚的变化（检测下降沿 - 按钮按下）
  for (int i = 1; i <= 3; i++) {
    if ((changed >> i) & 1) {
      bool newState = (currentState >> i) & 1;
      
      // 只处理下降沿（HIGH -> LOW，按钮按下）
      if (!newState) {
        switch (i) {
          case 1: // IO1 - 上一曲
            if (currentTime - lastIO1Change > DEBOUNCE_DELAY) {
              previousTrack();
              lastIO1Change = currentTime;
            }
            break;
            
          case 2: // IO2 - 暂停/播放
            if (currentTime - lastIO2Change > DEBOUNCE_DELAY) {
              togglePlayPause();
              lastIO2Change = currentTime;
            }
            break;
            
          case 3: // IO3 - 下一曲
            if (currentTime - lastIO3Change > DEBOUNCE_DELAY) {
              nextTrack();
              lastIO3Change = currentTime;
            }
            break;
        }
      }
    }
  }
  
  lastIOState = currentState;
}

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
  
  // 配置所有 IO 为输入 (0xFF = 所有位为 1 = 所有 IO 为输入)
  if (!ioExpander.portMode(0xFF)) {
    return false;
  }
  
  // 读取初始状态
  uint8_t initialState = 0;
  if (!ioExpander.digitalReadPort(initialState)) {
    return false;
  }
  lastIOState = initialState;
  
  // 配置中断（下降沿触发）
  attachInterrupt(digitalPinToInterrupt(INT_PIN), handlePCA9554Interrupt, FALLING);
  
  return true;
}

/**
 * 更新PCA9554状态
 */
void updatePCA9554() {
  // 检查中断标志
  if (interruptTriggered) {
    interruptTriggered = false;
    
    // 读取当前 IO 状态
    uint8_t currentState = 0;
    if (ioExpander.digitalReadPort(currentState)) {
      handleIOChange(currentState);
    }
  }
}

