#include "QMI8658A.h"

QMI8658A imu;

// 手势检测参数
float tiltThreshold = 0.5;     // 倾斜阈值 (g)
float shakeThreshold = 2.0;    // 摇晃阈值 (g)
float rotationThreshold = 100; // 旋转阈值 (dps)

unsigned long lastGesture = 0;
const unsigned long gestureDelay = 800; // 手势间隔

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("QMI8658A Gesture Control for Bluetooth Speaker");
    Serial.println("==============================================");
    
    // 初始化IMU
    imu.begin(0x6B);
    
    // 配置传感器
    imu.setAccScale(acc_scale_4g);      // ±4g量程
    imu.setGyroScale(gyro_scale_512dps); // ±512dps量程
    imu.setState(sensor_locking);        // 启用数据锁定模式
    
    Serial.println("Gesture controls:");
    Serial.println("- Tilt Left: Volume Down");
    Serial.println("- Tilt Right: Volume Up");
    Serial.println("- Shake: Play/Pause");
    Serial.println("- Rotate: Next/Previous Track");
    Serial.println();
}

void loop() {
    if (millis() - lastGesture > gestureDelay) {
        detectGestures();
    }
    delay(50);
}

void detectGestures() {
    float acc_x = imu.getAccX();
    float acc_y = imu.getAccY();
    float acc_z = imu.getAccZ();
    
    float gyro_x = imu.getGyroX();
    float gyro_y = imu.getGyroY();
    float gyro_z = imu.getGyroZ();
    
    // 1. 检测左右倾斜 (音量控制)
    if (abs(acc_y) > tiltThreshold && abs(acc_x) < 0.3) {
        if (acc_y > 0) {
            Serial.println("🔊 GESTURE: Tilt Right - Volume Up");
            // 这里可以调用蓝牙音箱的音量增加函数
        } else {
            Serial.println("🔉 GESTURE: Tilt Left - Volume Down");
            // 这里可以调用蓝牙音箱的音量减少函数
        }
        lastGesture = millis();
        return;
    }
    
    // 2. 检测摇晃 (播放/暂停)
    float totalAcc = sqrt(acc_x*acc_x + acc_y*acc_y + acc_z*acc_z);
    if (totalAcc > shakeThreshold) {
        Serial.println("⏯️  GESTURE: Shake - Play/Pause Toggle");
        // 这里可以调用蓝牙音箱的播放/暂停函数
        lastGesture = millis();
        return;
    }
    
    // 3. 检测旋转 (切换歌曲)
    if (abs(gyro_z) > rotationThreshold) {
        if (gyro_z > 0) {
            Serial.println("⏭️  GESTURE: Rotate Right - Next Track");
            // 这里可以调用蓝牙音箱的下一首函数
        } else {
            Serial.println("⏮️  GESTURE: Rotate Left - Previous Track");
            // 这里可以调用蓝牙音箱的上一首函数
        }
        lastGesture = millis();
        return;
    }
}