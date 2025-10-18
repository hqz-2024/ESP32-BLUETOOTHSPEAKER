#include "QMI8658A.h"

QMI8658A imu;

// 蓝牙音箱控制相关
bool isPlaying = false;
int currentVolume = 50; // 0-100
unsigned long lastGestureTime = 0;
const unsigned long gestureDebounce = 1000;

// 姿态检测参数
struct GestureThresholds {
    float tiltVolume = 0.4;      // 音量控制倾斜阈值
    float shakePlayPause = 1.8;  // 播放暂停摇晃阈值
    float rotateTrack = 80;      // 切歌旋转阈值
    float doubleTapPower = 2.5;  // 双击开关机阈值
} thresholds;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("QMI8658A + Bluetooth Speaker Integration");
    Serial.println("========================================");
    
    // 初始化IMU
    if (!initializeIMU()) {
        Serial.println("❌ IMU initialization failed!");
        while(1) delay(1000);
    }
    
    // 初始化蓝牙音箱 (这里是示例，需要根据您的具体实现)
    initializeBluetoothSpeaker();
    
    Serial.println("✅ System ready!");
    printGestureHelp();
}

void loop() {
    if (millis() - lastGestureTime > 50) { // 20Hz检测频率
        processGestures();
        lastGestureTime = millis();
    }
    
    // 其他蓝牙音箱处理逻辑
    delay(10);
}

bool initializeIMU() {
    imu.begin(0x6B);
    
    // 配置传感器参数
    imu.setAccScale(acc_scale_4g);
    imu.setGyroScale(gyro_scale_512dps);
    imu.setAccODR(acc_odr_norm_500);
    imu.setGyroODR(gyro_odr_norm_500);
    imu.setState(sensor_locking);
    
    delay(100);
    
    // 验证传感器工作
    float test_acc = imu.getAccZ();
    return (abs(test_acc) > 0.5 && abs(test_acc) < 2.0); // 重力应该接近1g
}

void initializeBluetoothSpeaker() {
    // 这里添加您的蓝牙音箱初始化代码
    Serial.println("🔵 Bluetooth speaker initialized");
}

void processGestures() {
    static unsigned long lastValidGesture = 0;
    
    if (millis() - lastValidGesture < gestureDebounce) {
        return; // 防抖动
    }
    
    // 读取传感器数据
    float acc_x = imu.getAccX();
    float acc_y = imu.getAccY();
    float acc_z = imu.getAccZ();
    float gyro_z = imu.getGyroZ();
    
    // 1. 音量控制 (左右倾斜)
    if (detectVolumeGesture(acc_y)) {
        lastValidGesture = millis();
        return;
    }
    
    // 2. 播放/暂停 (摇晃)
    if (detectPlayPauseGesture(acc_x, acc_y, acc_z)) {
        lastValidGesture = millis();
        return;
    }
    
    // 3. 切歌 (旋转)
    if (detectTrackChangeGesture(gyro_z)) {
        lastValidGesture = millis();
        return;
    }
}

bool detectVolumeGesture(float acc_y) {
    if (abs(acc_y) > thresholds.tiltVolume) {
        if (acc_y > 0) {
            // 右倾 - 音量增加
            currentVolume = min(100, currentVolume + 5);
            setVolume(currentVolume);
            Serial.printf("🔊 Volume Up: %d%%\n", currentVolume);
        } else {
            // 左倾 - 音量减少
            currentVolume = max(0, currentVolume - 5);
            setVolume(currentVolume);
            Serial.printf("🔉 Volume Down: %d%%\n", currentVolume);
        }
        return true;
    }
    return false;
}

bool detectPlayPauseGesture(float acc_x, float acc_y, float acc_z) {
    float magnitude = sqrt(acc_x*acc_x + acc_y*acc_y + acc_z*acc_z);
    
    if (magnitude > thresholds.shakePlayPause) {
        isPlaying = !isPlaying;
        togglePlayPause();
        Serial.printf("⏯️  %s\n", isPlaying ? "Playing" : "Paused");
        return true;
    }
    return false;
}

bool detectTrackChangeGesture(float gyro_z) {
    if (abs(gyro_z) > thresholds.rotateTrack) {
        if (gyro_z > 0) {
            nextTrack();
            Serial.println("⏭️  Next Track");
        } else {
            previousTrack();
            Serial.println("⏮️  Previous Track");
        }
        return true;
    }
    return false;
}

// 蓝牙音箱控制函数 (需要根据您的具体实现修改)
void setVolume(int volume) {
    // 实现音量控制
    Serial.printf("Setting volume to %d%%\n", volume);
}

void togglePlayPause() {
    // 实现播放/暂停切换
    Serial.println("Toggling play/pause");
}

void nextTrack() {
    // 实现下一首
    Serial.println("Switching to next track");
}

void previousTrack() {
    // 实现上一首
    Serial.println("Switching to previous track");
}

void printGestureHelp() {
    Serial.println("\n📱 Gesture Controls:");
    Serial.println("   🔊 Tilt Right  → Volume Up");
    Serial.println("   🔉 Tilt Left   → Volume Down");
    Serial.println("   ⏯️  Shake       → Play/Pause");
    Serial.println("   ⏭️  Rotate Right → Next Track");
    Serial.println("   ⏮️  Rotate Left  → Previous Track");
    Serial.println();
}