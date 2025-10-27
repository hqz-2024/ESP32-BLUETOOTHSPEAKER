/**
 * PCA9554 IO 变化检测程序
 * 
 * 功能：
 * 1. 初始化 PCA9554 (地址 0x38)
 * 2. 配置所有 IO 为输入模式
 * 3. 使用 INT 引脚 (GPIO2) 检测 IO 变化
 * 4. 当检测到 IO 变化时，输出对应的串口信息
 * 
 * 地址说明：
 * - 7位地址: 0x38 (Arduino Wire 库使用)
 * - 8位读地址: 0x70 (规格书)
 * - 8位写地址: 0x71 (规格书)
 * 
 * @author ESP32-BLUETOOTHSPEAKER 项目
 * @date 2024
 */

#include <Arduino.h>
#include <Wire.h>
#include <PCA9554.h>

// ==================== 配置参数 ====================

// I2C 引脚配置
#define I2C_SDA_PIN 4      // SDA 引脚
#define I2C_SCL_PIN 15     // SCL 引脚
#define I2C_FREQ 100000    // I2C 频率 (100kHz)

// PCA9554 配置
#define PCA9554_ADDR 0x38  // 7位 I2C 地址

// 中断引脚配置
#define INT_PIN 2          // INT 引脚 (GPIO2)

// ==================== 全局变量 ====================

// PCA9554 对象
PCA9554 ioExpander(PCA9554_ADDR);

// 上次的 IO 状态
uint8_t lastIOState = 0xFF;

// 中断标志
volatile bool interruptTriggered = false;

// ==================== 函数声明 ====================
void initializeHardware();
bool initializePCA9554();
void setupInterrupt();
void IRAM_ATTR handleInterrupt();
void checkIOChanges();
void printIOState(uint8_t state);
void printBinary(uint8_t value);

// ==================== 主程序 ====================

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待串口稳定
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   PCA9554 IO 变化检测程序            ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // 初始化硬件
  initializeHardware();
  
  // 初始化 PCA9554
  if (!initializePCA9554()) {
    Serial.println("❌ PCA9554 初始化失败！");
    Serial.println("   请检查：");
    Serial.println("   1. I2C 连接是否正常");
    Serial.println("   2. 设备地址是否正确 (0x38)");
    Serial.println("   3. 上拉电阻是否存在");
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("✅ PCA9554 初始化成功！");
  Serial.printf("   7位地址: 0x%02X\n", PCA9554_ADDR);
  Serial.printf("   8位写地址: 0x%02X\n", (PCA9554_ADDR << 1));
  Serial.printf("   8位读地址: 0x%02X\n", (PCA9554_ADDR << 1) | 1);
  
  // 设置中断
  setupInterrupt();
  
  Serial.println("\n✅ 中断已启用 (GPIO2)");
  Serial.println("等待 IO 变化...\n");
  
  // 读取初始状态
  uint8_t initialState = 0;
  ioExpander.digitalReadPort(initialState);
  lastIOState = initialState;
  
  Serial.println("初始 IO 状态:");
  printIOState(initialState);
  Serial.println("\n========================================\n");
}

void loop() {
  // 检查中断标志
  if (interruptTriggered) {
    interruptTriggered = false;
    
    // 延迟以消除抖动
    delay(20);
    
    // 检查 IO 变化
    checkIOChanges();
  }
  
  delay(10);
}

// ==================== 函数实现 ====================

/**
 * 初始化硬件
 */
void initializeHardware() {
  Serial.println("🔧 初始化硬件...");
  
  // 初始化 I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  Serial.printf("✅ I2C 初始化: SDA=%d, SCL=%d, 频率=%dHz\n", 
                I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  
  // 配置 INT 引脚为输入
  pinMode(INT_PIN, INPUT_PULLUP);
  Serial.printf("✅ INT 引脚配置: GPIO%d (INPUT_PULLUP)\n", INT_PIN);
}

/**
 * 初始化 PCA9554
 */
bool initializePCA9554() {
  Serial.println("\n🚀 初始化 PCA9554...");
  
  // 初始化库
  if (!ioExpander.begin()) {
    return false;
  }
  
  // 配置所有 IO 为输入 (1 = 输入, 0 = 输出)
  // 0xFF = 所有位为 1 = 所有 IO 为输入
  if (!ioExpander.portMode(0xFF)) {
    Serial.println("❌ 配置 IO 模式失败");
    return false;
  }
  
  Serial.println("   配置寄存器: 0xFF (11111111) - 所有 IO 为输入");
  
  return true;
}

/**
 * 设置中断
 */
void setupInterrupt() {
  Serial.println("\n⚙️  设置中断...");
  
  // 配置下降沿触发中断
  attachInterrupt(digitalPinToInterrupt(INT_PIN), handleInterrupt, FALLING);
  
  Serial.printf("✅ 中断已附加到 GPIO%d (下降沿触发)\n", INT_PIN);
}

/**
 * 中断处理函数
 */
void IRAM_ATTR handleInterrupt() {
  interruptTriggered = true;
}

/**
 * 检查 IO 变化
 */
void checkIOChanges() {
  // 读取当前 IO 状态
  uint8_t currentState = 0;
  if (!ioExpander.digitalReadPort(currentState)) {
    Serial.println("❌ 读取 IO 状态失败");
    return;
  }
  
  // 检查是否有变化
  if (currentState != lastIOState) {
    uint8_t changed = currentState ^ lastIOState;
    
    Serial.println("----------------------------------------");
    Serial.printf("时间: %lu ms\n", millis());
    Serial.println("🔔 检测到 IO 变化！");
    
    printIOState(currentState);
    
    // 显示变化的引脚
    Serial.println("\n变化的引脚:");
    for (int i = 0; i < 8; i++) {
      if ((changed >> i) & 1) {
        bool oldState = (lastIOState >> i) & 1;
        bool newState = (currentState >> i) & 1;
        Serial.printf("  IO%d: %s → %s\n", 
                      i,
                      oldState ? "HIGH" : "LOW ",
                      newState ? "HIGH" : "LOW ");
      }
    }
    
    Serial.println("----------------------------------------\n");
    
    lastIOState = currentState;
  }
}

/**
 * 打印 IO 状态
 */
void printIOState(uint8_t state) {
  Serial.print("IO 状态: 0x");
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
}

/**
 * 打印二进制数
 */
void printBinary(uint8_t value) {
  for (int i = 7; i >= 0; i--) {
    Serial.print((value >> i) & 1);
  }
}

