/*
 * PCA9554 简单IO控制例程
 * 适合初学者使用
 */

#include <PCA9554.h>

PCA9554 ioExpander(0x20);  // 创建PCA9554对象

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("PCA9554 简单IO控制");
    Serial.println("==================");
    
    // 初始化
    if (!ioExpander.begin()) {
        Serial.println("PCA9554初始化失败！");
        while(1) delay(1000);
    }
    
    Serial.println("PCA9554初始化成功！");
    
    // 配置引脚0-3为输入，4-7为输出
    for (int pin = 0; pin < 4; pin++) {
        ioExpander.pinMode(pin, INPUT);   // 输入引脚
    }
    for (int pin = 4; pin < 8; pin++) {
        ioExpander.pinMode(pin, OUTPUT);  // 输出引脚
    }
    
    Serial.println("引脚配置完成：");
    Serial.println("- 引脚0-3: 输入");
    Serial.println("- 引脚4-7: 输出");
}

void loop() {
    // 读取输入引脚状态
    for (int inputPin = 0; inputPin < 4; inputPin++) {
        bool inputState;
        if (ioExpander.digitalRead(inputPin, inputState)) {
            // 将输入状态映射到对应的输出引脚
            int outputPin = inputPin + 4;
            ioExpander.digitalWrite(outputPin, inputState);
            
            // 打印状态变化
            static bool lastStates[4] = {false, false, false, false};
            if (inputState != lastStates[inputPin]) {
                Serial.printf("输入%d: %s → 输出%d: %s\n", 
                    inputPin, inputState ? "HIGH" : "LOW",
                    outputPin, inputState ? "HIGH" : "LOW");
                lastStates[inputPin] = inputState;
            }
        }
    }
    
    delay(50);  // 50ms刷新率
}