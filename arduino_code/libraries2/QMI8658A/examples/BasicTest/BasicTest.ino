#include "QMI8658A.h"

QMI8658A imu;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("QMI8658A Basic Test");
    Serial.println("==================");
    
    // 初始化传感器 (默认地址0x6B)
    imu.begin(0x6B);
    
    Serial.println("Sensor initialized!");
    Serial.println("Acc_X, Acc_Y, Acc_Z, Gyro_X, Gyro_Y, Gyro_Z, Temp");
}

void loop() {
    // 读取所有传感器数据
    float acc_x = imu.getAccX();
    float acc_y = imu.getAccY(); 
    float acc_z = imu.getAccZ();
    
    float gyro_x = imu.getGyroX();
    float gyro_y = imu.getGyroY();
    float gyro_z = imu.getGyroZ();
    
    float temperature = imu.getTemp();
    
    // 输出数据
    Serial.print(acc_x, 3); Serial.print(", ");
    Serial.print(acc_y, 3); Serial.print(", ");
    Serial.print(acc_z, 3); Serial.print(", ");
    Serial.print(gyro_x, 2); Serial.print(", ");
    Serial.print(gyro_y, 2); Serial.print(", ");
    Serial.print(gyro_z, 2); Serial.print(", ");
    Serial.println(temperature, 1);
    
    delay(100);
}