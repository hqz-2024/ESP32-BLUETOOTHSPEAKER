/**
 * 配置管理模块实现
 * 
 * 负责蓝牙配对信息的保存和加载
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#include "config_manager.h"
#include "Preferences.h"

// Preferences对象
static Preferences preferences;

/**
 * 保存蓝牙配置信息
 */
void saveBluetoothConfig() {
  preferences.begin("bluetooth", false);
  preferences.putBool("paired", true);
  preferences.end();
  Serial.println("蓝牙配置已保存");
}

/**
 * 加载蓝牙配置信息
 */
bool loadBluetoothConfig() {
  preferences.begin("bluetooth", true);
  bool paired = preferences.getBool("paired", false);
  preferences.end();
  return paired;
}

/**
 * 清除蓝牙配置信息
 */
void clearBluetoothConfig() {
  preferences.begin("bluetooth", false);
  preferences.clear();
  preferences.end();
  Serial.println("蓝牙配置已清除");
}

