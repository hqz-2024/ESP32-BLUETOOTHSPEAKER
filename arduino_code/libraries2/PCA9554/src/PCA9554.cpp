#include "PCA9554.h"

PCA9554::PCA9554(byte SlaveAddress)
{
	_SlaveAddress = SlaveAddress;
	_initialized = false;
}

bool PCA9554::begin()
{
	Wire.begin();
	_initialized = isConnected();
	return _initialized;
}

bool PCA9554::isConnected()
{
	Wire.beginTransmission(_SlaveAddress);
	return (Wire.endTransmission() == 0);
}

bool PCA9554::twiRead(byte &registerAddress)
{
	Wire.beginTransmission(_SlaveAddress);
  	Wire.write(registerAddress);

  	if(Wire.endTransmission() == 0)
  	{
    		delay(15);
    		Wire.requestFrom(_SlaveAddress, 1, true);
    		while(Wire.available() < 1);
    		registerAddress = Wire.read();
    		return true;
  	}
  	return false;
}

bool PCA9554::twiWrite(byte registerAddress, byte dataWrite)
{
	Wire.beginTransmission(_SlaveAddress);
  	Wire.write(registerAddress);
  	Wire.write(dataWrite);

  	if(Wire.endTransmission() == 0)
    		return true;
  	return false;
}

bool PCA9554::pinMode(byte pinNumber, bool state)
{
	byte oldValue = CONFIGPORT;
	if(this->twiRead(oldValue) && (pinNumber <= 7))
	{
		if(!state)
		{
			oldValue |= (1 << pinNumber);
			if(this->portMode(oldValue))
				return true;
			return false;
		}
		else if(state)
		{
			oldValue &= ~(1 << pinNumber);
			if(this->portMode(oldValue))
				return true;
			return false;
		}
	}
	return false;
}

bool PCA9554::portMode(byte value)
{
	if(this->twiWrite(CONFIGPORT, value))
		return true;
	return false;
}


bool PCA9554::digitalWrite(byte pinNumber, bool state)
{
	if (!_initialized || pinNumber > 7) return false;  // 改进：检查初始化和引脚范围
	
	byte oldValue = OUTPUTPORT;
	if(this->twiRead(oldValue))
	{
		if(state)
		{
			oldValue |= (1 << pinNumber);
		}
		else
		{
			oldValue &= ~(1 << pinNumber);
		}
		return this->digitalWritePort(oldValue);
	}
	return false;
}



bool PCA9554::digitalWritePort(byte value)
{
	if(this->twiWrite(OUTPUTPORT, value))
		return true;
	return false;
}



bool PCA9554::digitalRead(byte pinNumber, bool &state)
{
	if (!_initialized || pinNumber > 7) return false;  // 改进：检查初始化和引脚范围
	
	byte oldValue = INPUTPORT;
	if(this->twiRead(oldValue))
	{
		state = (oldValue & (1 << pinNumber)) ? true : false;
		return true;
	}
	return false;
}


bool PCA9554::digitalReadPort(byte &value)
{
	value = INPUTPORT;
	if(this->twiRead(value))
		return true;
	return false;
}

bool PCA9554::setPinPolarity(byte pinNumber, bool inverted)
{
	if (!_initialized || pinNumber > 7) return false;
	
	byte oldValue = POLINVPORT;
	if(this->twiRead(oldValue))
	{
		if(inverted)
		{
			oldValue |= (1 << pinNumber);
		}
		else
		{
			oldValue &= ~(1 << pinNumber);
		}
		return this->twiWrite(POLINVPORT, oldValue);
	}
	return false;
}
