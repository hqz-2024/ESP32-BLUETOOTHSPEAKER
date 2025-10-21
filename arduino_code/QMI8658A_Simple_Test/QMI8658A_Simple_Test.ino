#include <Arduino.h>
#include <Wire.h>
#include "QMI8658A.h"

// I2C引脚定义
#define I2C_SDA_PIN 4   // SDA引脚
#define I2C_SCL_PIN 15  // SCL引脚
#define QMI8658A_ADDR 0x6A  // QMI8658A默认I2C地址

// 创建QMI8658A实例
QMI8658A imu;

// 状态变量
bool sensorInitialized = false;
unsigned long lastDataOutput = 0;
unsigned long lastStatusCheck = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("========================================");
    Serial.println("      QMI8658A 简单测试程序");
    Serial.println("========================================");
    Serial.printf("I2C配置: SDA=GPIO%d, SCL=GPIO%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.printf("设备地址: 0x%02X\n", QMI8658A_ADDR);
    Serial.println("========================================");
    
    // 初始化硬件
    initializeHardware();
    
    // 初始化传感器
    if (initializeSensor()) {
        sensorInitialized = true;
        Serial.println("✅ QMI8658A初始化成功!");
        printDataHeader();
    } else {
        Serial.println("❌ QMI8658A初始化失败!");
        Serial.println("请检查:");
        Serial.println("  1. 硬件连接是否正确");
        Serial.println("  2. I2C上拉电阻是否安装");
        Serial.println("  3. 设备供电是否正常");
    }
}

void loop() {
    if (sensorInitialized) {
        // 输出传感器数据
        outputSensorData();
        
        // 定期状态检查
        periodicStatusCheck();
    } else {
        // 尝试重新初始化
        static unsigned long lastRetry = 0;
        if (millis() - lastRetry > 5000) {
            Serial.println("🔄 尝试重新初始化传感器...");
            if (initializeSensor()) {
                sensorInitialized = true;
                Serial.println("✅ 传感器重新初始化成功!");
                printDataHeader();
            }
            lastRetry = millis();
        }
    }
    
    delay(10);
}

void initializeHardware() {
    Serial.println("\n🔧 初始化硬件...");
    
    // 初始化I2C总线
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(400000); // 400kHz
    
    delay(100);
    
    // 检查引脚状态
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);
    delay(10);
    
    bool sda_ok = digitalRead(I2C_SDA_PIN);
    bool scl_ok = digitalRead(I2C_SCL_PIN);
    
    Serial.printf("SDA引脚: %s\n", sda_ok ? "HIGH ✅" : "LOW ❌");
    Serial.printf("SCL引脚: %s\n", scl_ok ? "HIGH ✅" : "LOW ❌");
    
    if (!sda_ok || !scl_ok) {
        Serial.println("⚠️  警告: 引脚状态异常，可能需要上拉电阻");
    }
    
    // 重新初始化I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(400000);
}

bool initializeSensor() {
    Serial.println("\n🚀 初始化QMI8658A传感器...");
    
    // 检查I2C通讯
    Wire.beginTransmission(QMI8658A_ADDR);
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        Serial.printf("❌ I2C通讯失败，错误代码: %d\n", error);
        printI2CError(error);
        return false;
    }
    
    Serial.println("✅ I2C通讯正常");
    
    // 初始化传感器
    try {
        imu.begin(QMI8658A_ADDR, 400000);
        
        // 配置传感器
        imu.setAccScale(acc_scale_4g);        // ±4g
        imu.setGyroScale(gyro_scale_1024dps); // ±1024°/s
        imu.setAccODR(acc_odr_norm_500);      // 500Hz
        imu.setGyroODR(gyro_odr_norm_500);    // 500Hz
        imu.setAccLPF(lpf_5_39);              // 低通滤波
        imu.setGyroLPF(lpf_5_39);
        imu.setState(sensor_running);         // 启动传感器
        
        delay(100);
        
        // 验证传感器工作
        return verifySensorOperation();
        
    } catch (...) {
        Serial.println("❌ 传感器初始化异常");
        return false;
    }
}

bool verifySensorOperation() {
    Serial.println("🔍 验证传感器工作状态...");
    
    // 读取几次数据验证
    float acc_readings[3];
    bool valid_readings = true;
    
    for (int i = 0; i < 3; i++) {
        float acc_x = imu.getAccX();
        float acc_y = imu.getAccY();
        float acc_z = imu.getAccZ();
        
        acc_readings[i] = sqrt(acc_x*acc_x + acc_y*acc_y + acc_z*acc_z);
        
        Serial.printf("  读取 %d: X=%.3f Y=%.3f Z=%.3f |%.3f|g\n", 
                     i+1, acc_x, acc_y, acc_z, acc_readings[i]);
        
        // 检查数据合理性 (重力加速度应该接近1g)
        if (acc_readings[i] < 0.5 || acc_readings[i] > 2.0) {
            valid_readings = false;
        }
        
        delay(50);
    }
    
    if (valid_readings) {
        Serial.println("✅ 传感器数据正常");
        return true;
    } else {
        Serial.println("❌ 传感器数据异常");
        return false;
    }
}

void printDataHeader() {
    Serial.println("\n📊 开始数据输出 (每500ms一次):");
    Serial.println("时间(ms)  |  加速度(g)           |  陀螺仪(°/s)         |  姿态角(°)");
    Serial.println("----------|---------------------|---------------------|-------------");
}

void outputSensorData() {
    if (millis() - lastDataOutput > 500) { // 每500ms输出一次
        // 读取传感器数据
        float acc_x = imu.getAccX();
        float acc_y = imu.getAccY();
        float acc_z = imu.getAccZ();
        float gyro_x = imu.getGyroX();
        float gyro_y = imu.getGyroY();
        float gyro_z = imu.getGyroZ();
        
        // 计算合成值
        float acc_magnitude = sqrt(acc_x*acc_x + acc_y*acc_y + acc_z*acc_z);
        float gyro_magnitude = sqrt(gyro_x*gyro_x + gyro_y*gyro_y + gyro_z*gyro_z);
        
        // 计算姿态角
        float pitch = atan2(acc_x, sqrt(acc_y*acc_y + acc_z*acc_z)) * 180.0 / PI;
        float roll = atan2(acc_y, sqrt(acc_x*acc_x + acc_z*acc_z)) * 180.0 / PI;
        
        // 格式化输出
        Serial.printf("%8lu  | %5.2f %5.2f %5.2f | %6.1f %6.1f %6.1f | %5.1f %5.1f\n",
                     millis(),
                     acc_x, acc_y, acc_z,
                     gyro_x, gyro_y, gyro_z,
                     pitch, roll);
        
        // 检测异常情况
        if (acc_magnitude < 0.3 || acc_magnitude > 3.0) {
            Serial.printf("⚠️  异常: 加速度幅值 %.3f g\n", acc_magnitude);
        }
        
        if (gyro_magnitude > 500) {
            Serial.printf("⚠️  高速旋转: %.1f °/s\n", gyro_magnitude);
        }
        
        lastDataOutput = millis();
    }
}

void periodicStatusCheck() {
    if (millis() - lastStatusCheck > 10000) { // 每10秒检查一次
        Serial.println("\n--- 状态检查 ---");
        
        // 检查I2C通讯
        Wire.beginTransmission(QMI8658A_ADDR);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.println("✅ I2C通讯正常");
        } else {
            Serial.printf("❌ I2C通讯异常，错误: %d\n", error);
            sensorInitialized = false;
        }
        
        // 检查数据稳定性
        float acc_z = imu.getAccZ();
        if (abs(acc_z) > 0.5 && abs(acc_z) < 2.0) {
            Serial.println("✅ 传感器数据稳定");
        } else {
            Serial.printf("⚠️  传感器数据不稳定: Z轴=%.3f\n", acc_z);
        }
        
        Serial.println("--- 检查完成 ---\n");
        lastStatusCheck = millis();
    }
}

void printI2CError(uint8_t error) {
    Serial.print("I2C错误详情: ");
    switch (error) {
        case 1:
            Serial.println("数据太长，超出传输缓冲区");
            break;
        case 2:
            Serial.println("在地址传输时收到NACK (设备未响应)");
            Serial.println("  - 检查设备地址是否正确");
            Serial.println("  - 检查设备是否正常供电");
            break;
        case 3:
            Serial.println("在数据传输时收到NACK");
            break;
        case 4:
            Serial.println("其他I2C错误");
            Serial.println("  - 检查SDA/SCL连接");
            Serial.println("  - 检查上拉电阻");
            break;
        default:
            Serial.println("未知错误");
            break;
    }
}

// 简单的手势检测功能
void detectSimpleGestures() {
    static float prev_acc_magnitude = 1.0;
    
    float acc_x = imu.getAccX();
    float acc_y = imu.getAccY();
    float acc_z = imu.getAccZ();
    float acc_magnitude = sqrt(acc_x*acc_x + acc_y*acc_y + acc_z*acc_z);
    
    // 检测摇晃
    if (abs(acc_magnitude - prev_acc_magnitude) > 0.5) {
        Serial.println("👋 检测到摇晃!");
    }
    
    // 检测倾斜
    if (abs(acc_y) > 0.7) {
        Serial.printf("📐 倾斜: %s\n", acc_y > 0 ? "右倾" : "左倾");
    }
    
    prev_acc_magnitude = acc_magnitude;
}
