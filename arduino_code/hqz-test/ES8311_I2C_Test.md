/**
 * @file ES8311_I2C_Test.ino
 * @brief ES8311 I2C测试程序
 * @author ES8311 Test
 * @copyright GPLv3
 */

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

// I2C设备扫描
void scanI2CDevices() {
  Serial.println("扫描I2C设备...");
  int deviceCount = 0;
  
  for (int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C设备发现，地址: 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      deviceCount++;
    }
  }
  
  if (deviceCount == 0) {
    Serial.println("没有发现I2C设备");
  } else {
    Serial.print("发现 ");
    Serial.print(deviceCount);
    Serial.println(" 个I2C设备");
  }
}

// ES8311 I2C写入函数
void writeES8311Register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  int error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.print("成功写入寄存器 0x");
    Serial.print(reg, HEX);
    Serial.print(" = 0x");
    Serial.println(value, HEX);
  } else {
    Serial.print("写入寄存器失败，错误码: ");
    Serial.println(error);
  }
  delay(1);
}

// ES8311 I2C读取函数
uint8_t readES8311Register(uint8_t reg) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  int error = Wire.endTransmission(false);
  
  if (error != 0) {
    Serial.print("读取寄存器失败，错误码: ");
    Serial.println(error);
    return 0;
  }
  
  Wire.requestFrom(ES8311_I2C_ADDR, 1);
  if (Wire.available()) {
    uint8_t value = Wire.read();
    Serial.print("从寄存器 0x");
    Serial.print(reg, HEX);
    Serial.print(" 读取: 0x");
    Serial.println(value, HEX);
    return value;
  } else {
    Serial.println("读取寄存器数据失败");
    return 0;
  }
}

// ES8311初始化配置
void initES8311() {
  Serial.println("正在初始化ES8311音频芯片...");
  
  // 检查ES8311是否存在
  Wire.beginTransmission(ES8311_I2C_ADDR);
  int error = Wire.endTransmission();
  if (error != 0) {
    Serial.println("ES8311未找到，请检查I2C连接");
    return;
  }
  
  // 复位ES8311
  Serial.println("复位ES8311...");
  writeES8311Register(ES8311_RESET_REG, 0x3F);
  delay(10);
  writeES8311Register(ES8311_RESET_REG, 0x00);
  delay(10);
  
  // 时钟管理配置
  Serial.println("配置时钟管理...");
  writeES8311Register(ES8311_CLK_MANAGER_REG, 0x30);
  
  // 模拟配置
  Serial.println("配置模拟前端...");
  writeES8311Register(ES8311_ANALOG_REG, 0x48);
  
  // 系统配置
  Serial.println("配置系统参数...");
  writeES8311Register(ES8311_SYSTEM_REG, 0x05);
  
  // 耳机输出配置
  Serial.println("配置耳机输出...");
  writeES8311Register(ES8311_HEADPHONE_REG, 0xC0);
  
  // ADC配置
  Serial.println("配置ADC...");
  writeES8311Register(ES8311_ADC_REG, 0x17);
  
  // DAC配置
  Serial.println("配置DAC...");
  writeES8311Register(ES8311_DAC_REG, 0x08);
  
  // 音量控制
  Serial.println("设置音量...");
  writeES8311Register(ES8311_VOLUME_REG, 0x00);
  
  delay(100);
  
  Serial.println("ES8311初始化完成");
  
  // 验证配置
  Serial.println("验证配置...");
  readES8311Register(ES8311_RESET_REG);
  readES8311Register(ES8311_CLK_MANAGER_REG);
  readES8311Register(ES8311_ANALOG_REG);
  readES8311Register(ES8311_SYSTEM_REG);
}

// 打印所有ES8311配置信息
void printES8311Config() {
  Serial.println("========== ES8311配置信息 ==========");
  
  Serial.print("复位寄存器 (0x00): 0x");
  Serial.println(readES8311Register(ES8311_RESET_REG), HEX);
  
  Serial.print("时钟管理寄存器 (0x01): 0x");
  Serial.println(readES8311Register(ES8311_CLK_MANAGER_REG), HEX);
  
  Serial.print("模拟寄存器 (0x02): 0x");
  Serial.println(readES8311Register(ES8311_ANALOG_REG), HEX);
  
  Serial.print("系统寄存器 (0x03): 0x");
  Serial.println(readES8311Register(ES8311_SYSTEM_REG), HEX);
  
  Serial.print("耳机寄存器 (0x04): 0x");
  Serial.println(readES8311Register(ES8311_HEADPHONE_REG), HEX);
  
  Serial.print("ADC寄存器 (0x09): 0x");
  Serial.println(readES8311Register(ES8311_ADC_REG), HEX);
  
  Serial.print("DAC寄存器 (0x0A): 0x");
  Serial.println(readES8311Register(ES8311_DAC_REG), HEX);
  
  Serial.print("音量寄存器 (0x0C): 0x");
  Serial.println(readES8311Register(ES8311_VOLUME_REG), HEX);
  
  Serial.println("===================================");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println("ES8311 I2C测试程序启动");
  
  // 初始化I2C总线
  Serial.println("初始化I2C总线...");
  Wire.begin(3, 4); // SDA=21, SCL=22 (ESP32默认I2C引脚)
  Wire.setClock(100000); // 设置I2C时钟为100kHz
  
  delay(1000);
  
  // 扫描I2C设备
  scanI2CDevices();
  
  delay(1000);
  
  // 初始化ES8311芯片
  initES8311();
  
  Serial.println("测试完成");
}

void loop() {
  // 打印所有ES8311配置信息
  printES8311Config();
  
  // 等待3秒后再次打印
  delay(3000);
} 