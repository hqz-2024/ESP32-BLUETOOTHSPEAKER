#ifndef QMI8658A_H
#define QMI8658A_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// 设备识别
#define QMI8658A_WHO_AM_I           0x00
#define QMI8658A_REVISION_ID        0x01
#define QMI8658A_DEVICE_ID          0x05
#define QMI8658A_REVISION_VALUE     0x7C

// 核心寄存器
#define QMI8658A_CTRL1              0x02
#define QMI8658A_CTRL2              0x03  // 加速度计配置
#define QMI8658A_CTRL3              0x04  // 陀螺仪配置
#define QMI8658A_CTRL7              0x08  // 传感器使能
#define QMI8658A_CTRL9              0x0A  // 命令寄存器
#define QMI8658A_RESET              0x60

// 状态寄存器
#define QMI8658A_STATUSINT          0x2D
#define QMI8658A_STATUS0            0x2E

// 数据寄存器
#define QMI8658A_AX_L               0x35
#define QMI8658A_AX_H               0x36
#define QMI8658A_AY_L               0x37
#define QMI8658A_AY_H               0x38
#define QMI8658A_AZ_L               0x39
#define QMI8658A_AZ_H               0x3A
#define QMI8658A_GX_L               0x3B
#define QMI8658A_GX_H               0x3C
#define QMI8658A_GY_L               0x3D
#define QMI8658A_GY_H               0x3E
#define QMI8658A_GZ_L               0x3F
#define QMI8658A_GZ_H               0x40
#define QMI8658A_TEMP_L             0x33
#define QMI8658A_TEMP_H             0x34

// I2C地址
#define QMI8658A_I2C_ADDR_SA0_HIGH  0x6A
#define QMI8658A_I2C_ADDR_SA0_LOW   0x6B

// 加速度计量程
typedef enum {
    QMI8658A_ACC_RANGE_2G = 0x00,
    QMI8658A_ACC_RANGE_4G = 0x01,
    QMI8658A_ACC_RANGE_8G = 0x02,
    QMI8658A_ACC_RANGE_16G = 0x03
} qmi8658a_acc_range_t;

// 陀螺仪量程
typedef enum {
    QMI8658A_GYRO_RANGE_16DPS = 0x00,
    QMI8658A_GYRO_RANGE_32DPS = 0x01,
    QMI8658A_GYRO_RANGE_64DPS = 0x02,
    QMI8658A_GYRO_RANGE_128DPS = 0x03,
    QMI8658A_GYRO_RANGE_256DPS = 0x04,
    QMI8658A_GYRO_RANGE_512DPS = 0x05,
    QMI8658A_GYRO_RANGE_1024DPS = 0x06,
    QMI8658A_GYRO_RANGE_2048DPS = 0x07
} qmi8658a_gyro_range_t;

// 数据结构
typedef struct {
    float x, y, z;
} qmi8658a_data_t;

class QMI8658A {
public:
    QMI8658A();
    
    // 初始化
    bool begin(uint8_t i2c_addr = QMI8658A_I2C_ADDR_SA0_HIGH, TwoWire *wire = &Wire);
    bool beginSPI(uint8_t cs_pin, SPIClass *spi = &SPI);
    
    // 基础功能
    bool isConnected();
    void reset();
    
    // 配置
    void enableAccelerometer(bool enable = true);
    void enableGyroscope(bool enable = true);
    void enableSyncMode(bool enable = true);
    void setAccelerometerRange(qmi8658a_acc_range_t range);
    void setGyroscopeRange(qmi8658a_gyro_range_t range);
    
    // 数据读取
    bool isDataReady();
    qmi8658a_data_t readAccelerometer();
    qmi8658a_data_t readGyroscope();
    float readTemperature();
    void readAll(qmi8658a_data_t *acc, qmi8658a_data_t *gyro, float *temp = nullptr);
    
    // 高级功能
    void calibrateGyroscope();
    bool selfTestAccelerometer();
    bool selfTestGyroscope();
    
private:
    bool _use_i2c;
    uint8_t _i2c_addr;
    uint8_t _cs_pin;
    TwoWire *_wire;
    SPIClass *_spi;
    
    qmi8658a_acc_range_t _acc_range;
    qmi8658a_gyro_range_t _gyro_range;
    
    // 通信函数
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);
    void readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length);
    
    // 工具函数
    int16_t combineBytes(uint8_t low, uint8_t high);
    float convertAcceleration(int16_t raw);
    float convertGyroscope(int16_t raw);
};

#endif