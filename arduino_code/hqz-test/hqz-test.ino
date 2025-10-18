/**
 * @file hqz-test.ino
 * @brief Copy audio from I2S to I2S with ES8311 codec configuration
 * @author Modified for ES8311 support
 * @copyright GPLv3
 */

#include "AudioTools.h"
#include <Wire.h>

// ES8311 I2C配置
#define ES8311_I2C_ADDR 0x18  // ES8311的I2C地址

// ES8311寄存器地址
#define ES8311_RESET_REG        0x00
#define ES8311_CLK_MANAGER_REG  0x01
#define ES8311_ANALOG_REG       0x02
#define ES8311_SYSTEM_REG       0x03
#define ES8311_HEADPHONE_REG    0x04
#define ES8311_ADC_REG          0x09
#define ES8311_DAC_REG          0x0A
#define ES8311_VOLUME_REG       0x0C
#define ES8311_ID_REG        0xFD
#define ES8311_ID2_REG       0xFE

AudioInfo info(16000, 1, 16);
I2SStream i2s;
StreamCopy copier(i2s, i2s); // copies sound into i2s

// ES8311配置监控变量
uint32_t lastConfigCheckTime = 0;
const uint32_t CONFIG_CHECK_INTERVAL = 3000; // 每3秒检查一次配置

// ES8311 I2C写入函数
void writeES8311Register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  delay(1); // 给芯片一些时间处理
}

// ES8311 I2C读取函数
uint8_t readES8311Register(uint8_t reg) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  
  Wire.requestFrom(ES8311_I2C_ADDR, 1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

// ES8311初始化配置
void initES8311() {
  Serial.println("正在初始化ES8311音频芯片...");
  
  // 复位ES8311
  writeES8311Register(ES8311_RESET_REG, 0x3F);
  delay(10);
  writeES8311Register(ES8311_RESET_REG, 0x00);
  delay(10);
  
  // 时钟管理配置
  // 设置MCLK分频器和时钟模式
  writeES8311Register(ES8311_CLK_MANAGER_REG, 0x30); // 启用时钟，设置合适的分频
  
  // 模拟配置
  // 配置ADC和DAC的模拟前端
  writeES8311Register(ES8311_ANALOG_REG, 0x48); // 启用ADC/DAC，设置增益
  
  // 系统配置
  // 配置采样率和数据格式
  writeES8311Register(ES8311_SYSTEM_REG, 0x05); // 16kHz采样率，16位数据
  
  // 耳机输出配置
  // 启用耳机输出，设置驱动强度
  writeES8311Register(ES8311_HEADPHONE_REG, 0xC0); // 启用耳机输出
  
  // ADC配置
  // 配置ADC增益和输入选择
  writeES8311Register(ES8311_ADC_REG, 0x17); // ADC增益设置
  
  // DAC配置
  // 配置DAC增益和输出
  writeES8311Register(ES8311_DAC_REG, 0x08); // DAC配置
  
  // 音量控制
  // 设置主音量
  writeES8311Register(ES8311_VOLUME_REG, 0x00); // 最大音量
  
  delay(100); // 等待配置稳定
  
  Serial.println("ES8311初始化完成");
  
  // 验证配置
  uint8_t chipId = readES8311Register(ES8311_RESET_REG);
  Serial.print("ES8311芯片ID读取: 0x");
  Serial.println(chipId, HEX);
}

// 打印ES8311配置状态
void printES8311Config() {
  Serial.println("========== ES8311配置状态检查 ==========");
  
  uint8_t regValue;
  
  // 读取并打印复位寄存器
  regValue = readES8311Register(ES8311_ID_REG);
  Serial.print("复位寄存器 (0x00): 0x");
  Serial.print(regValue, HEX);


  regValue = readES8311Register(ES8311_ID2_REG);
  Serial.print("复位寄存器 (0x00): 0x");
  Serial.print(regValue, HEX);

  
  
  Serial.println("========================================");
  Serial.println();
}

// Arduino Setup
void setup(void) {  
  // Open Serial 
  Serial.begin(115200);
  // change to Warning to improve the quality
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Error); 

  // 初始化I2C总线
  Serial.println("初始化I2C总线...");
  Wire.begin(3, 4); // SDA=21, SCL=22 (ESP32默认I2C引脚)
  Wire.setClock(100000); // 设置I2C时钟为100kHz
  
  // 初始化ES8311芯片
  initES8311();

  // start I2S
  Serial.println("启动I2S...");
  auto config = i2s.defaultConfig(RXTX_MODE);
  config.copyFrom(info); 
  config.i2s_format = I2S_STD_FORMAT;
  config.pin_ws = 6;
  config.pin_bck = 8;
  config.pin_data = 5;
  config.pin_data_rx = 7;

  config.fixed_mclk = 16000 * 256;
  config.pin_mck = 10; // must be 0,1 or 3 - only for ESP_IDF_VERSION_MAJOR >= 4
  i2s.begin(config);

  Serial.println("I2S启动完成，ES8311配置就绪");
}

// Arduino loop - copy sound to out 
void loop() {
  // 执行音频数据复制
  copier.copy();
  
  // 定期检查ES8311配置状态
  uint32_t currentTime = millis();
  if (currentTime - lastConfigCheckTime >= CONFIG_CHECK_INTERVAL) {
    printES8311Config();
    lastConfigCheckTime = currentTime;
  }
}