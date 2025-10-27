/**
 * PCA9554A IO 输入读取程序
 * 
 * 功能：
 * 1. 初始化 PCA9554A (地址 0x38)
 * 2. 配置所有 IO 为输入模式
 * 3. 不断读取 IO 输入状态
 * 4. 通过串口打印 IO 状态
 * 
 * @author ESP32-BLUETOOTHSPEAKER 项目
 * @date 2024
 */

#include <Arduino.h>
#include <Wire.h>

// ==================== 配置参数 ====================

// I2C 引脚配置
#define I2C_SDA_PIN 4      // SDA 引脚
#define I2C_SCL_PIN 15     // SCL 引脚
#define I2C_FREQ 100000    // I2C 频率 (100kHz)

// PCA9554A 配置
#define PCA9554A_ADDR 0x38  // 7位 I2C 地址 (对应 8位地址 0x70/0x71)

// PCA9554A 寄存器地址
#define REG_INPUT    0x00   // 输入寄存器
#define REG_OUTPUT   0x01   // 输出寄存器
#define REG_POLARITY 0x02   // 极性反转寄存器
#define REG_CONFIG   0x03   // 配置寄存器 (1=输入, 0=输出)

// 读取间隔（毫秒）
#define READ_INTERVAL 500   // 每 500ms 读取一次

// ==================== 全局变量 ====================
uint8_t lastInputState = 0xFF;  // 上次的输入状态
unsigned long lastReadTime = 0;  // 上次读取时间

// ==================== 函数声明 ====================
bool initPCA9554A();
uint8_t readPCA9554A_Input();
void printIOState(uint8_t state);
void printBinary(uint8_t value);

// ==================== 主程序 ====================

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待串口稳定
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   PCA9554A IO 输入读取程序            ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // 初始化 I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  Serial.printf("I2C 初始化: SDA=%d, SCL=%d, 频率=%dHz\n", 
                I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  
  // 初始化 PCA9554A
  Serial.println("\n正在初始化 PCA9554A...");
  if (initPCA9554A()) {
    Serial.println("✅ PCA9554A 初始化成功！");
    Serial.printf("   7位地址: 0x%02X\n", PCA9554A_ADDR);
    Serial.printf("   8位写地址: 0x%02X\n", (PCA9554A_ADDR << 1));
    Serial.printf("   8位读地址: 0x%02X\n", (PCA9554A_ADDR << 1) | 1);
  } else {
    Serial.println("❌ PCA9554A 初始化失败！");
    Serial.println("   请检查：");
    Serial.println("   1. I2C 连接是否正常");
    Serial.println("   2. 设备地址是否正确");
    Serial.println("   3. 上拉电阻是否存在");
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("\n开始读取 IO 输入状态...");
  Serial.println("========================================\n");
  
  lastReadTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  
  // 按设定间隔读取
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;
    
    // 读取输入状态
    uint8_t inputState = readPCA9554A_Input();
    
    // 只有状态改变时才打印详细信息
    if (inputState != lastInputState) {
      Serial.println("----------------------------------------");
      Serial.printf("时间: %lu ms\n", currentTime);
      printIOState(inputState);
      Serial.println("----------------------------------------\n");
      lastInputState = inputState;
    } else {
      // 状态未改变，只打印简单信息
      Serial.print(".");
    }
  }
}

// ==================== 函数实现 ====================

/**
 * 初始化 PCA9554A
 * 配置所有 IO 为输入模式
 */
bool initPCA9554A() {
  // 检查设备是否存在
  Wire.beginTransmission(PCA9554A_ADDR);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  // 配置所有 IO 为输入 (写入 0xFF 到配置寄存器)
  Wire.beginTransmission(PCA9554A_ADDR);
  Wire.write(REG_CONFIG);
  Wire.write(0xFF);  // 所有位设为 1 = 输入模式
  if (Wire.endTransmission() != 0) {
    return false;
  }
  
  delay(10);
  
  // 验证配置
  Wire.beginTransmission(PCA9554A_ADDR);
  Wire.write(REG_CONFIG);
  Wire.endTransmission();
  
  if (Wire.requestFrom(PCA9554A_ADDR, (uint8_t)1) == 1) {
    uint8_t config = Wire.read();
    Serial.printf("   配置寄存器: 0x%02X (", config);
    printBinary(config);
    Serial.println(")");
    return (config == 0xFF);
  }
  
  return false;
}

/**
 * 读取 PCA9554A 输入寄存器
 */
uint8_t readPCA9554A_Input() {
  Wire.beginTransmission(PCA9554A_ADDR);
  Wire.write(REG_INPUT);
  
  if (Wire.endTransmission() != 0) {
    Serial.println("❌ 写入寄存器地址失败");
    return 0xFF;
  }
  
  if (Wire.requestFrom(PCA9554A_ADDR, (uint8_t)1) == 1) {
    return Wire.read();
  } else {
    Serial.println("❌ 读取输入寄存器失败");
    return 0xFF;
  }
}

/**
 * 打印 IO 状态
 */
void printIOState(uint8_t state) {
  Serial.print("输入状态: 0x");
  Serial.print(state, HEX);
  Serial.print(" (0b");
  printBinary(state);
  Serial.println(")");
  
  Serial.println("\n各引脚状态:");
  Serial.println("  引脚 | 状态 | 电平");
  Serial.println("  -----|------|------");
  
  for (int i = 0; i < 8; i++) {
    bool pinState = (state >> i) & 1;
    Serial.printf("  IO%d  |  %d   | %s\n", 
                  i, 
                  pinState ? 1 : 0,
                  pinState ? "HIGH" : "LOW ");
  }
  
  // 显示变化的引脚
  if (lastInputState != 0xFF) {
    uint8_t changed = state ^ lastInputState;
    if (changed != 0) {
      Serial.println("\n变化的引脚:");
      for (int i = 0; i < 8; i++) {
        if ((changed >> i) & 1) {
          bool oldState = (lastInputState >> i) & 1;
          bool newState = (state >> i) & 1;
          Serial.printf("  IO%d: %s → %s\n", 
                        i,
                        oldState ? "HIGH" : "LOW ",
                        newState ? "HIGH" : "LOW ");
        }
      }
    }
  }
}

/**
 * 打印二进制数
 */
void printBinary(uint8_t value) {
  for (int i = 7; i >= 0; i--) {
    Serial.print((value >> i) & 1);
  }
}

