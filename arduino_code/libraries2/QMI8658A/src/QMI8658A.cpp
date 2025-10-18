#include "QMI8658A.h"

QMI8658A::QMI8658A() {
    _use_i2c = true;
    _i2c_addr = QMI8658A_I2C_ADDR_SA0_HIGH;
    _cs_pin = 0;
    _wire = nullptr;
    _spi = nullptr;
    _acc_range = QMI8658A_ACC_RANGE_2G;
    _gyro_range = QMI8658A_GYRO_RANGE_256DPS;
}

bool QMI8658A::begin(uint8_t i2c_addr, TwoWire *wire) {
    _use_i2c = true;
    _i2c_addr = i2c_addr;
    _wire = wire;
    
    _wire->begin();
    delay(50);
    
    // 检查设备ID
    if (!isConnected()) {
        return false;
    }
    
    // 软复位
    reset();
    delay(15);
    
    // 基础配置
    writeRegister(QMI8658A_CTRL1, 0x40); // 地址自增
    writeRegister(QMI8658A_CTRL2, 0x04); // 加速度计500Hz, ±2g
    writeRegister(QMI8658A_CTRL3, 0x04); // 陀螺仪448.4Hz, ±256dps
    writeRegister(QMI8658A_CTRL7, 0x83); // 使能6轴+同步模式
    
    return true;
}

bool QMI8658A::beginSPI(uint8_t cs_pin, SPIClass *spi) {
    _use_i2c = false;
    _cs_pin = cs_pin;
    _spi = spi;
    
    pinMode(_cs_pin, OUTPUT);
    digitalWrite(_cs_pin, HIGH);
    
    _spi->begin();
    _spi->setDataMode(SPI_MODE0);
    _spi->setClockDivider(SPI_CLOCK_DIV16); // 1MHz
    
    delay(50);
    
    if (!isConnected()) {
        return false;
    }
    
    reset();
    delay(15);
    
    // SPI模式配置
    writeRegister(QMI8658A_CTRL1, 0x40);
    writeRegister(QMI8658A_CTRL2, 0x04);
    writeRegister(QMI8658A_CTRL3, 0x04);
    writeRegister(QMI8658A_CTRL7, 0x83);
    
    return true;
}

bool QMI8658A::isConnected() {
    uint8_t who_am_i = readRegister(QMI8658A_WHO_AM_I);
    return (who_am_i == QMI8658A_DEVICE_ID);
}

void QMI8658A::reset() {
    writeRegister(QMI8658A_RESET, 0xB0);
}

void QMI8658A::enableAccelerometer(bool enable) {
    uint8_t ctrl7 = readRegister(QMI8658A_CTRL7);
    if (enable) {
        ctrl7 |= 0x01;
    } else {
        ctrl7 &= ~0x01;
    }
    writeRegister(QMI8658A_CTRL7, ctrl7);
}

void QMI8658A::enableGyroscope(bool enable) {
    uint8_t ctrl7 = readRegister(QMI8658A_CTRL7);
    if (enable) {
        ctrl7 |= 0x02;
    } else {
        ctrl7 &= ~0x02;
    }
    writeRegister(QMI8658A_CTRL7, ctrl7);
}

void QMI8658A::enableSyncMode(bool enable) {
    uint8_t ctrl7 = readRegister(QMI8658A_CTRL7);
    if (enable) {
        ctrl7 |= 0x80;
    } else {
        ctrl7 &= ~0x80;
    }
    writeRegister(QMI8658A_CTRL7, ctrl7);
}

void QMI8658A::setAccelerometerRange(qmi8658a_acc_range_t range) {
    _acc_range = range;
    uint8_t ctrl2 = readRegister(QMI8658A_CTRL2);
    ctrl2 = (ctrl2 & 0x8F) | (range << 4);
    writeRegister(QMI8658A_CTRL2, ctrl2);
}

void QMI8658A::setGyroscopeRange(qmi8658a_gyro_range_t range) {
    _gyro_range = range;
    uint8_t ctrl3 = readRegister(QMI8658A_CTRL3);
    ctrl3 = (ctrl3 & 0x8F) | (range << 4);
    writeRegister(QMI8658A_CTRL3, ctrl3);
}

bool QMI8658A::isDataReady() {
    uint8_t status = readRegister(QMI8658A_STATUSINT);
    return (status & 0x01) != 0;
}

qmi8658a_data_t QMI8658A::readAccelerometer() {
    uint8_t data[6];
    readRegisters(QMI8658A_AX_L, data, 6);
    
    qmi8658a_data_t result;
    result.x = convertAcceleration(combineBytes(data[0], data[1]));
    result.y = convertAcceleration(combineBytes(data[2], data[3]));
    result.z = convertAcceleration(combineBytes(data[4], data[5]));
    
    return result;
}

qmi8658a_data_t QMI8658A::readGyroscope() {
    uint8_t data[6];
    readRegisters(QMI8658A_GX_L, data, 6);
    
    qmi8658a_data_t result;
    result.x = convertGyroscope(combineBytes(data[0], data[1]));
    result.y = convertGyroscope(combineBytes(data[2], data[3]));
    result.z = convertGyroscope(combineBytes(data[4], data[5]));
    
    return result;
}

float QMI8658A::readTemperature() {
    uint8_t data[2];
    readRegisters(QMI8658A_TEMP_L, data, 2);
    
    int16_t raw = combineBytes(data[0], data[1]);
    return raw / 256.0f; // 根据规格书转换
}

void QMI8658A::readAll(qmi8658a_data_t *acc, qmi8658a_data_t *gyro, float *temp) {
    // 同步模式下先读状态寄存器触发锁存
    readRegister(QMI8658A_STATUSINT);
    
    if (acc) *acc = readAccelerometer();
    if (gyro) *gyro = readGyroscope();
    if (temp) *temp = readTemperature();
}

void QMI8658A::calibrateGyroscope() {
    writeRegister(QMI8658A_CTRL9, 0xA2); // 陀螺仪校准命令
    delay(500); // 等待校准完成
}

// 私有函数实现
uint8_t QMI8658A::readRegister(uint8_t reg) {
    if (_use_i2c) {
        _wire->beginTransmission(_i2c_addr);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(_i2c_addr, (uint8_t)1);
        return _wire->read();
    } else {
        digitalWrite(_cs_pin, LOW);
        _spi->transfer(reg | 0x80); // 读取位
        uint8_t value = _spi->transfer(0x00);
        digitalWrite(_cs_pin, HIGH);
        return value;
    }
}

void QMI8658A::writeRegister(uint8_t reg, uint8_t value) {
    if (_use_i2c) {
        _wire->beginTransmission(_i2c_addr);
        _wire->write(reg);
        _wire->write(value);
        _wire->endTransmission();
    } else {
        digitalWrite(_cs_pin, LOW);
        _spi->transfer(reg & 0x7F); // 写入位
        _spi->transfer(value);
        digitalWrite(_cs_pin, HIGH);
    }
}

void QMI8658A::readRegisters(uint8_t reg, uint8_t *buffer, uint8_t length) {
    if (_use_i2c) {
        _wire->beginTransmission(_i2c_addr);
        _wire->write(reg);
        _wire->endTransmission(false);
        _wire->requestFrom(_i2c_addr, length);
        for (uint8_t i = 0; i < length; i++) {
            buffer[i] = _wire->read();
        }
    } else {
        digitalWrite(_cs_pin, LOW);
        _spi->transfer(reg | 0x80);
        for (uint8_t i = 0; i < length; i++) {
            buffer[i] = _spi->transfer(0x00);
        }
        digitalWrite(_cs_pin, HIGH);
    }
}

int16_t QMI8658A::combineBytes(uint8_t low, uint8_t high) {
    return (int16_t)((high << 8) | low);
}

float QMI8658A::convertAcceleration(int16_t raw) {
    float scale;
    switch (_acc_range) {
        case QMI8658A_ACC_RANGE_2G:  scale = 2.0f / 32768.0f; break;
        case QMI8658A_ACC_RANGE_4G:  scale = 4.0f / 32768.0f; break;
        case QMI8658A_ACC_RANGE_8G:  scale = 8.0f / 32768.0f; break;
        case QMI8658A_ACC_RANGE_16G: scale = 16.0f / 32768.0f; break;
        default: scale = 2.0f / 32768.0f;
    }
    return raw * scale;
}

float QMI8658A::convertGyroscope(int16_t raw) {
    float scale;
    switch (_gyro_range) {
        case QMI8658A_GYRO_RANGE_16DPS:   scale = 16.0f / 32768.0f; break;
        case QMI8658A_GYRO_RANGE_32DPS:   scale = 32.0f / 32768.0f; break;
        case QMI8658A_GYRO_RANGE_64DPS:   scale = 64.0f / 32768.0f; break;
        case QMI8658A_GYRO_RANGE_128DPS:  scale = 128.0f / 32768.0f; break;
        case QMI8658A_GYRO_RANGE_256DPS:  scale = 256.0f / 32768.0f; break;
        case QMI8658A_GYRO_RANGE_512DPS:  scale = 512.0f / 32768.0f; break;
        case QMI8658A_GYRO_RANGE_1024DPS: scale = 1024.0f / 32768.0f; break;
        case QMI8658A_GYRO_RANGE_2048DPS: scale = 2048.0f / 32768.0f; break;
        default: scale = 256.0f / 32768.0f;
    }
    return raw * scale;
}