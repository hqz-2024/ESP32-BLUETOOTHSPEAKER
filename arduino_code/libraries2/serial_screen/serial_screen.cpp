#include "serial_screen.h"

// 初始化静态成员变量
bool SerialScreen::is_set_pure_mode = false;

SerialScreen::SerialScreen(HardwareSerial* serial, const EmotionControlPair* map, size_t size) : _serial(serial), emotionMap(map), emotionMapSize(size) {
  Serial.printf("设置情绪映射表：大小=%zu\n", size);
}

// 辅助函数：根据情绪名称查找对应的控件ID
uint16_t findControlIdByEmotionName(const char* emotionName, const EmotionControlPair* emotionMap, size_t mapSize) {
  if (!emotionName || !emotionMap) {
    return 0;
  }
  
  // 遍历emotionMap查找匹配的情绪
  for (size_t i = 0; i < mapSize; i++) {
    if (strcmp(emotionName, emotionMap[i].emotionName) == 0) {
      return emotionMap[i].controlId;
    }
  }
  
  return CONTROL_ING_ANIMATION; // 未找到返回ING动画
}

void SerialScreen::set_pure_mode() {
  is_set_pure_mode = true;
}

void SerialScreen::begin(unsigned long baudrate, uint32_t config, int8_t rxPin, int8_t txPin) {
  if (_serial) {
    _serial->begin(baudrate, config, rxPin, txPin);
  }
}


/*********************************文本控件处理***************************  */
void SerialScreen::clearBottomText(uint16_t screenId, uint16_t controlId) {
  if (!_serial || is_set_pure_mode) return;
  
  // 将十进制参数转换为高低字节
  uint8_t screenIdHigh = (screenId >> 8) & 0xFF;
  uint8_t screenIdLow = screenId & 0xFF;
  uint8_t controlIdHigh = (controlId >> 8) & 0xFF;
  uint8_t controlIdLow = controlId & 0xFF;
  
  Serial.println("发送清空下方文字指令：画面编号" + String(screenId) + ", 控件编号" + String(controlId));
  
  // 构建指令: AA B1 10 Screen_id Control_id CC 33 C3 3C
  byte command[] = {
    0xAA, 0xB1, 0x10, 
    screenIdHigh, screenIdLow, 
    controlIdHigh, controlIdLow, 
    0xCC, 0x33, 0xC3, 0x3C
  };
  _serial->write(command, sizeof(command));
}

void SerialScreen::updateText(uint16_t screenId, uint16_t controlId, const char* text) {
  if (!_serial || !text||is_set_pure_mode) return;
  
  // 将十进制参数转换为高低字节
  uint8_t screenIdHigh = (screenId >> 8) & 0xFF;
  uint8_t screenIdLow = screenId & 0xFF;
  uint8_t controlIdHigh = (controlId >> 8) & 0xFF;
  uint8_t controlIdLow = controlId & 0xFF;
  
  const size_t MAX_TEXT_LENGTH = 200;
  byte commandBuffer[9 + MAX_TEXT_LENGTH + 4]; // 9个固定字节 + 最大文本长度 + 4字节帧尾
  
  // 确保文本长度不超过缓冲区容量
  size_t textLength = strlen(text);
  size_t safeTextLength = min(textLength, MAX_TEXT_LENGTH);
  
  // Serial.println("发送文本更新指令：画面编号" + String(screenId) + ", 控件编号" + String(controlId) + ", 文本内容：" + String(text));
  
  // 构建指令头部: AA B1 10 Screen_id Control_id Strings CC 33 C3 3C
  int index = 0;
  commandBuffer[index++] = 0xAA; // 帧头 AA
  commandBuffer[index++] = 0xB1; // 指令 B1
  commandBuffer[index++] = 0x10; // 参数 10 (更新文本显示)
  commandBuffer[index++] = screenIdHigh; // Screen_id 高8位
  commandBuffer[index++] = screenIdLow; // Screen_id 低8位
  commandBuffer[index++] = controlIdHigh; // Control_id 高8位
  commandBuffer[index++] = controlIdLow; // Control_id 低8位
  
  // // 添加文本内容
  // for (size_t i = 0; i < safeTextLength; i++) {
  //   commandBuffer[index++] = text[i];
  // }
   // UTF-8转UTF-16BE编码
  size_t textIndex = 0;
  while (text[textIndex] != 0 && index < sizeof(commandBuffer) - 4) {
    uint8_t firstByte = (uint8_t)text[textIndex];
    
    if (firstByte < 0x80) {
      // 单字节ASCII字符 (0-127)
      // UTF-16BE表示为: 00 [ASCII值]
      commandBuffer[index++] = 0x00;
      commandBuffer[index++] = firstByte;
      textIndex++;
    } else if ((firstByte & 0xE0) == 0xC0) {
      // 双字节UTF-8字符 (192-223)
      if (text[textIndex+1] && index + 1 < sizeof(commandBuffer) - 4) {
        uint16_t unicode = ((firstByte & 0x1F) << 6) | ((uint8_t)text[textIndex+1] & 0x3F);
        // UTF-16BE格式：高字节在前，低字节在后
        commandBuffer[index++] = (unicode >> 8) & 0xFF; // 高字节
        commandBuffer[index++] = unicode & 0xFF;        // 低字节
        textIndex += 2;
      } else {
        break;
      }
    } else if ((firstByte & 0xF0) == 0xE0) {
      // 三字节UTF-8字符 (224-239) - 大部分中文字符
      if (text[textIndex+1] && text[textIndex+2] && index + 1 < sizeof(commandBuffer) - 4) {
        uint16_t unicode = ((firstByte & 0x0F) << 12) |
                          (((uint8_t)text[textIndex+1] & 0x3F) << 6) |
                          ((uint8_t)text[textIndex+2] & 0x3F);
        // UTF-16BE格式：高字节在前，低字节在后
        commandBuffer[index++] = (unicode >> 8) & 0xFF; // 高字节
        commandBuffer[index++] = unicode & 0xFF;        // 低字节
        textIndex += 3;
      } else {
        break;
      }
    } else {
      // 四字节UTF-8字符或其他情况 - 暂时跳过
      textIndex++;
    }
  }

  // 帧尾
  commandBuffer[index++] = 0xCC;
  commandBuffer[index++] = 0x33;
  commandBuffer[index++] = 0xC3;
  commandBuffer[index++] = 0x3C;
  
  // 发送指令
  _serial->write(commandBuffer, index);
}


/**************************图标类控件播放：电池电量、音量状态、WIFI信号强度********************/

void SerialScreen::AlarmLogo(uint16_t screenId, uint16_t controlId, uint8_t iconImageId) {
  // 如果是简洁模式，不显示logo
  if (is_set_pure_mode) {
    iconImageId = 0;
  }  
  // 使用showIcon方法显示logo
  // 画面编号: screenId, 控件编号: controlId, 图标帧ID: iconImageId
  showIcon(screenId, controlId, iconImageId);
}

void SerialScreen::ChargingLogo(uint16_t screenId, uint16_t controlId, uint8_t iconImageId) {
  if (!_serial) return;
  
  // 如果是简洁模式，不显示logo
  if (is_set_pure_mode) {
    iconImageId = 0;
  }
  
  // 使用showIcon方法显示充电状态
  // 画面编号: 1, 控件编号: 11, 图标帧ID: 1
  showIcon(screenId, controlId, iconImageId);
}

void SerialScreen::showBatteryLevel( uint16_t screenId, uint16_t controlId,int batteryLevel) {
  if (!_serial) return;
  
  // 将电量百分比(0-100)映射到1-4格显示
  int batteryBars;
  if (batteryLevel <= 0) batteryBars = 1;           // 0及以下 -> 1格
  else if (batteryLevel <= 25) batteryBars = 1;     // 1-25% -> 1格
  else if (batteryLevel <= 50) batteryBars = 2;     // 26-50% -> 2格
  else if (batteryLevel <= 75) batteryBars = 3;     // 51-75% -> 3格
  else batteryBars = 4;                             // 76-100% -> 4格
  
  // 如果是简洁模式，强制电池显示为0
  if (is_set_pure_mode) {
    batteryBars = 0;
  }  
  // 使用showIcon方法显示电池电量图标
  showIcon(screenId, controlId, (uint8_t)batteryBars);
}

void SerialScreen::showVolume(uint16_t screenId, uint16_t controlId,float volumeState) {
  if (!_serial) return;
  
  // 获取音量值并映射到四个状态(1-4)
  // 使用更直接的映射方法确保音量状态能正确分布在1-4范围内
    // 如果是0-1范围的值，使用更直接的映射方式
    if (volumeState <= 0) volumeState = 1;           // 0 -> 1
    else if (volumeState <= 0.33) volumeState = 1;  // 0-0.33 -> 1
    else if (volumeState <= 0.66) volumeState = 2;  // 0.34-0.66 -> 2
    else if (volumeState <= 0.9) volumeState = 3;   // 0.67-0.9 -> 3
    else volumeState = 4;                           // 0.91-1.0 -> 4
  
  // 如果是简洁模式，强制音量显示为0
  if (is_set_pure_mode) {
    volumeState = 0;
  }
  // 使用showIcon方法显示音量图标
  // 画面编号: 0x0001 (1), 控件编号: 0x0003 (3), 图标帧ID: volumeState
  showIcon(screenId, controlId, (uint8_t)volumeState);
}

void SerialScreen::showSignalLevel(uint16_t screenId,uint16_t controlId,int signalLevel) {
  if (!_serial) return;
  
  // 如果是简洁模式，强制信号强度显示为0
  if (is_set_pure_mode) {
    signalLevel = 0;
  }
  // 使用showIcon方法显示信号强度图标
  showIcon(screenId, controlId, (uint8_t)signalLevel);
}

/**
 * @brief 显示图标
 * @param screenId 画面编号
 * @param controlId 控件编号
 * @param iconImageId 图标帧ID
 */
void SerialScreen::showIcon(uint16_t screenId, uint16_t controlId, uint8_t iconImageId) {
  if (!_serial) return;
  
  // 将十进制参数转换为高低字节
  uint8_t screenIdHigh = (screenId >> 8) & 0xFF;
  uint8_t screenIdLow = screenId & 0xFF;
  uint8_t controlIdHigh = (controlId >> 8) & 0xFF;
  uint8_t controlIdLow = controlId & 0xFF;
  
  // Serial.println("发送图标显示指令：画面编号[0x" + String(screenIdHigh, HEX) + ",0x" + String(screenIdLow, HEX) + "]，控件编号[0x" + 
  //               String(controlIdHigh, HEX) + ",0x" + String(controlIdLow, HEX) + "]，图标帧ID[0x" + String(iconImageId, HEX) + "]");
  
  // 构建图标显示指令：AA B1 23 SCREEN_ID_CONTROL_ID ICON_IMAGE_ID CC 33 C3 3C
  // 其中SCREEN_ID和CONTROL_ID各占2个字节，ICON_IMAGE_ID占1个字节
  byte command[] = {
    0xAA, 0xB1, 0x23, 
    screenIdHigh, screenIdLow,  // 画面编号
    controlIdHigh, controlIdLow, // 控件编号
    iconImageId,                // 图标帧ID
    0xCC, 0x33, 0xC3, 0x3C
  };
  
  _serial->write(command, sizeof(command));
}
/*******************************************************/


/**
 * @brief 切换画面
 * @param screenId 画面编号
 */
void SerialScreen::switchScreen(uint16_t screenId) {
  if (!_serial) return;  
  
  // 切换画面指令: AA B1 00 Screen_id CC 33 C3 3C  
  byte command[] = {  
    0xAA, 0xB1,0x00,(byte)(screenId >> 8), (byte)(screenId & 0xFF), 0xCC, 0x33, 0xC3, 0x3C  
  };  
  // Serial.println("发送切换画面指令：画面编号[0x" + String(screenId, HEX) + "]");  
  
  // // 打印完整指令内容
  // String commandHex = "完整指令: ";
  // for (size_t i = 0; i < sizeof(command); i++) {
  //   commandHex += "0x" + String(command[i], HEX);
  //   if (i < sizeof(command) - 1) {
  //     commandHex += " ";
  //   }
  // }
  // Serial.println(commandHex);
  
  _serial->write(command, sizeof(command));  
}

/**
 * @brief 切换动画显示状态
 * @param screenId 画面编号
 * @param controlId 控件编号
 * @param hide 是否显示动画
 */
void SerialScreen::AnimationVisibility(uint16_t screenId, uint16_t controlId, bool hide) {
  if (!_serial) return;
  
  // 将十进制参数转换为高低字节
  uint8_t screenIdHigh = (screenId >> 8) & 0xFF;
  uint8_t screenIdLow = screenId & 0xFF;
  uint8_t controlIdHigh = (controlId >> 8) & 0xFF;
  uint8_t controlIdLow = controlId & 0xFF;
  
  // 设置HideFlag：0表示显示，1表示隐藏
  uint8_t hideFlag = hide ? 0x00 : 0x01;
  
  // Serial.println("发送动画控件" + String(hide ? "显示" : "隐藏") + "指令：画面编号" + String(screenId) + ", 控件编号" + String(controlId));  
  // 构建显示/隐藏动画控件指令：AA B1 2A Screen_id Control_id HideFlag CC 33 C3 3C
  byte command[] = {
    0xAA, 0xB1, 0x2A, 
    screenIdHigh, screenIdLow,  // 画面编号
    controlIdHigh, controlIdLow, // 控件编号
    hideFlag,                   // 隐藏标志
    0xCC, 0x33, 0xC3, 0x3C
  };
  
  _serial->write(command, sizeof(command));
}
/**
 * @brief 播放动画
 * @param screenId 画面编号
 * @param controlId 控件编号
 * @param isPlay 是否播放动画
 */
/**
 * @brief 播放动画
 * @param screenId 画面编号
 * @param controlId 控件编号
 * @param isPlay 是否播放动画
 */
void SerialScreen::playAnimation(uint16_t screenId, uint16_t controlId, bool isPlay) {
  if (!_serial) return;
  
  // 将十进制参数转换为高低字节
  uint8_t screenIdHigh = (screenId >> 8) & 0xFF;
  uint8_t screenIdLow = screenId & 0xFF;
  uint8_t controlIdHigh = (controlId >> 8) & 0xFF;
  uint8_t controlIdLow = controlId & 0xFF;
  
  // 设置指令第三个字节：播放为0x20，停止为0x21
  uint8_t commandType = isPlay ? 0x20 : 0x21;
  
  // Serial.println("发送动画" + String(isPlay ? "播放" : "停止") + "指令：画面编号" + String(screenId) + ", 控件编号" + String(controlId));
  
  // 构建动画播放/停止指令：AA B1 [commandType] Screen_id Control_id CC 33 C3 3C
  byte command[] = {
    0xAA, 0xB1, commandType, 
    screenIdHigh, screenIdLow,      // 画面编号
    controlIdHigh, controlIdLow,    // 控件编号
    0xCC, 0x33, 0xC3, 0x3C
  };
  
  _serial->write(command, sizeof(command));
}

/**
 * @brief 渲染指定情绪动画
 * @param screenId 画面编号
 * @param prevEmoName 上个状态的情绪名称
 * @param newEmoName 需要刷新的新情绪名称
 */
void SerialScreen::renderEmotionByName(uint16_t screenId, const char* prevEmoName, const char* newEmoName) {
  if (!_serial || is_set_pure_mode) return;
  // Serial.println("渲染指定情绪动画：画面编号" + String(screenId) + ", 上个状态" + String(prevEmoName) + ", 新状态" + String(newEmoName));
  // 查找对应的控件ID
  uint16_t prevControlId = findControlIdByEmotionName(prevEmoName, emotionMap, emotionMapSize);
  uint16_t newControlId = findControlIdByEmotionName(newEmoName, emotionMap, emotionMapSize);
  
  // Serial.printf("渲染情绪动画：从[%s]到[%s] (控件ID: 0x%04X -> 0x%04X)\n", 
  //              prevEmoName ? prevEmoName : "空", 
  //              newEmoName ? newEmoName : "空",
  //              prevControlId, newControlId);
  
  // 停止并隐藏上个状态的动画
  if (prevControlId != 0 && prevControlId != newControlId) {
    playAnimation(screenId, prevControlId, false); // 停止动画
    AnimationVisibility(screenId, prevControlId, false); // 隐藏动画
    // Serial.printf("停止并隐藏上个状态动画：控件ID[0x%04X]\n", prevControlId);
  }
  
  // 显示并播放新状态的动画
  if (newControlId != 0) {
    AnimationVisibility(screenId, newControlId, true); // 显示动画
    playAnimation(screenId, newControlId, true); // 播放动画
    // Serial.printf("显示并播放新状态动画：控件ID[0x%04X]\n", newControlId);
  }
}

