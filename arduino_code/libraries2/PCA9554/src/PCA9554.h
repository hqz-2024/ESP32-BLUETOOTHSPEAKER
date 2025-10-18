
#ifndef PCA9554_H_INCLUDED
#define PCA9554_H_INCLUDED

#include <Arduino.h>
#include <Wire.h>

// PCA9554 寄存器地址
#define INPUTPORT	0x00
#define OUTPUTPORT	0x01
#define POLINVPORT	0x02
#define CONFIGPORT	0x03

// 端口配置常量
#define ALLOUTPUT	0x00
#define ALLINPUT	0xFF

class PCA9554
{
    public:
        PCA9554(byte SlaveAddress);
        
        // 初始化函数
        bool begin();
        bool isConnected();

        // 底层I2C通信
        bool twiRead(byte &registerAddress);
        bool twiWrite(byte registerAddress, byte dataWrite);

        // 引脚配置
        bool pinMode(byte pinNumber, bool state);
        bool portMode(byte value);
        
        // 数字输出
        bool digitalWrite(byte pinNumber, bool state);
        bool digitalWritePort(byte value);
                
        // 数字输入
        bool digitalRead(byte pinNumber, bool &state);
        bool digitalReadPort(byte &value);
        
        // 极性反转
        bool setPinPolarity(byte pinNumber, bool inverted);
        bool setPortPolarity(byte value);
        
    private:
        int _SlaveAddress;
        bool _initialized;
};

#endif
