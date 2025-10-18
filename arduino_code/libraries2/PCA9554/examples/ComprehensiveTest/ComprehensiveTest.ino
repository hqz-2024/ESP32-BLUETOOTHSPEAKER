/*
 * PCA9554 综合测试例程
 * 演示所有功能：输入、输出、极性反转、端口操作
 */

#include <PCA9554.h>
#include <Wire.h>

// 创建PCA9554对象 (地址0x20)
PCA9554 ioExpander(0x20);

// 测试配置
const int TEST_DELAY = 1000;
bool testPassed = true;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("PCA9554 综合功能测试");
    Serial.println("====================");
    
    // 初始化I2C和PCA9554
    if (!ioExpander.begin()) {
        Serial.println("❌ PCA9554初始化失败！检查连接和地址。");
        while(1) {
            delay(1000);
        }
    }
    
    Serial.println("✅ PCA9554初始化成功！");
    
    // 运行所有测试
    runAllTests();
    
    if (testPassed) {
        Serial.println("\n🎉 所有测试通过！开始演示模式...\n");
        setupDemoMode();
    } else {
        Serial.println("\n❌ 部分测试失败，请检查硬件连接。");
    }
}

void loop() {
    if (testPassed) {
        runDemoMode();
    } else {
        // 如果测试失败，持续检查连接
        if (ioExpander.isConnected()) {
            Serial.println("🔄 设备重新连接，重启测试...");
            delay(1000);
            ESP.restart(); // 重启进行重新测试
        }
        delay(5000);
    }
}

void runAllTests() {
    Serial.println("\n1️⃣ 测试设备连接...");
    testConnection();
    
    Serial.println("\n2️⃣ 测试输出功能...");
    testOutputFunctions();
    
    Serial.println("\n3️⃣ 测试输入功能...");
    testInputFunctions();
    
    Serial.println("\n4️⃣ 测试端口操作...");
    testPortOperations();
    
    Serial.println("\n5️⃣ 测试极性反转...");
    testPolarityInversion();
}

void testConnection() {
    if (ioExpander.isConnected()) {
        Serial.println("   ✅ 设备连接正常");
    } else {
        Serial.println("   ❌ 设备连接失败");
        testPassed = false;
    }
}

void testOutputFunctions() {
    Serial.println("   配置所有引脚为输出模式...");
    
    if (!ioExpander.portMode(ALLOUTPUT)) {
        Serial.println("   ❌ 端口配置失败");
        testPassed = false;
        return;
    }
    
    Serial.println("   测试单个引脚输出...");
    for (int pin = 0; pin < 8; pin++) {
        // 测试高电平
        if (!ioExpander.digitalWrite(pin, HIGH)) {
            Serial.printf("   ❌ 引脚%d输出HIGH失败\n", pin);
            testPassed = false;
        }
        delay(100);
        
        // 测试低电平
        if (!ioExpander.digitalWrite(pin, LOW)) {
            Serial.printf("   ❌ 引脚%d输出LOW失败\n", pin);
            testPassed = false;
        }
        delay(100);
    }
    
    Serial.println("   ✅ 输出功能测试完成");
}

void testInputFunctions() {
    Serial.println("   配置引脚0-3为输入，4-7为输出...");
    
    // 配置引脚0-3为输入
    for (int pin = 0; pin < 4; pin++) {
        if (!ioExpander.pinMode(pin, INPUT)) {
            Serial.printf("   ❌ 引脚%d配置为输入失败\n", pin);
            testPassed = false;
        }
    }
    
    // 配置引脚4-7为输出
    for (int pin = 4; pin < 8; pin++) {
        if (!ioExpander.pinMode(pin, OUTPUT)) {
            Serial.printf("   ❌ 引脚%d配置为输出失败\n", pin);
            testPassed = false;
        }
    }
    
    Serial.println("   读取输入引脚状态（请连接上拉电阻或按钮）：");
    for (int pin = 0; pin < 4; pin++) {
        bool state;
        if (ioExpander.digitalRead(pin, state)) {
            Serial.printf("   引脚%d: %s\n", pin, state ? "HIGH" : "LOW");
        } else {
            Serial.printf("   ❌ 读取引脚%d失败\n", pin);
            testPassed = false;
        }
    }
    
    Serial.println("   ✅ 输入功能测试完成");
}

void testPortOperations() {
    Serial.println("   测试端口批量操作...");
    
    // 配置所有引脚为输出
    ioExpander.portMode(ALLOUTPUT);
    
    // 测试不同的端口值
    byte testPatterns[] = {0x00, 0xFF, 0xAA, 0x55, 0x0F, 0xF0};
    int patternCount = sizeof(testPatterns) / sizeof(testPatterns[0]);
    
    for (int i = 0; i < patternCount; i++) {
        if (!ioExpander.digitalWritePort(testPatterns[i])) {
            Serial.printf("   ❌ 端口写入0x%02X失败\n", testPatterns[i]);
            testPassed = false;
        } else {
            Serial.printf("   ✅ 端口写入0x%02X成功\n", testPatterns[i]);
        }
        delay(200);
    }
    
    Serial.println("   ✅ 端口操作测试完成");
}

void testPolarityInversion() {
    Serial.println("   测试极性反转功能...");
    
    // 配置所有引脚为输出
    ioExpander.portMode(ALLOUTPUT);
    
    // 设置引脚0-3为正常极性，4-7为反转极性
    for (int pin = 0; pin < 4; pin++) {
        if (!ioExpander.setPinPolarity(pin, false)) {
            Serial.printf("   ❌ 引脚%d极性设置失败\n", pin);
            testPassed = false;
        }
    }
    
    for (int pin = 4; pin < 8; pin++) {
        if (!ioExpander.setPinPolarity(pin, true)) {
            Serial.printf("   ❌ 引脚%d极性反转设置失败\n", pin);
            testPassed = false;
        }
    }
    
    // 测试极性反转效果
    Serial.println("   写入0xFF，引脚0-3应为HIGH，引脚4-7应为LOW");
    ioExpander.digitalWritePort(0xFF);
    delay(1000);
    
    Serial.println("   写入0x00，引脚0-3应为LOW，引脚4-7应为HIGH");
    ioExpander.digitalWritePort(0x00);
    delay(1000);
    
    // 恢复正常极性
    ioExpander.setPortPolarity(0x00);
    
    Serial.println("   ✅ 极性反转测试完成");
}

void setupDemoMode() {
    Serial.println("演示模式设置：");
    Serial.println("- 引脚0-3: 输入模式（连接按钮或开关）");
    Serial.println("- 引脚4-7: 输出模式（连接LED）");
    
    // 配置引脚模式
    for (int pin = 0; pin < 4; pin++) {
        ioExpander.pinMode(pin, INPUT);
    }
    for (int pin = 4; pin < 8; pin++) {
        ioExpander.pinMode(pin, OUTPUT);
    }
    
    // 清除所有输出
    for (int pin = 4; pin < 8; pin++) {
        ioExpander.digitalWrite(pin, LOW);
    }
}

void runDemoMode() {
    static unsigned long lastUpdate = 0;
    static int ledPattern = 0;
    
    // 每500ms更新一次
    if (millis() - lastUpdate > 500) {
        // 读取输入状态并控制对应的LED
        for (int inputPin = 0; inputPin < 4; inputPin++) {
            bool inputState;
            if (ioExpander.digitalRead(inputPin, inputState)) {
                int ledPin = inputPin + 4;
                ioExpander.digitalWrite(ledPin, inputState);
                
                if (inputState) {
                    Serial.printf("🔘 按钮%d按下 → LED%d点亮\n", inputPin, ledPin);
                }
            }
        }
        
        lastUpdate = millis();
    }
    
    // 如果没有按钮输入，运行LED流水灯效果
    static unsigned long lastLedUpdate = 0;
    if (millis() - lastLedUpdate > 200) {
        bool anyButtonPressed = false;
        
        // 检查是否有按钮被按下
        for (int pin = 0; pin < 4; pin++) {
            bool state;
            if (ioExpander.digitalRead(pin, state) && state) {
                anyButtonPressed = true;
                break;
            }
        }
        
        // 如果没有按钮按下，运行流水灯
        if (!anyButtonPressed) {
            runLedChaser();
        }
        
        lastLedUpdate = millis();
    }
}

void runLedChaser() {
    static int currentLed = 4;
    static bool direction = true;
    
    // 清除所有LED
    for (int pin = 4; pin < 8; pin++) {
        ioExpander.digitalWrite(pin, LOW);
    }
    
    // 点亮当前LED
    ioExpander.digitalWrite(currentLed, HIGH);
    
    // 移动到下一个LED
    if (direction) {
        currentLed++;
        if (currentLed > 7) {
            currentLed = 6;
            direction = false;
        }
    } else {
        currentLed--;
        if (currentLed < 4) {
            currentLed = 5;
            direction = true;
        }
    }
}