/*
 Battery.cpp - Battery library
 Copyright (c) 2014 Roberto Lo Giacco.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Battery.h"
#include <Arduino.h>

Battery::Battery(uint16_t minVoltage, uint16_t maxVoltage, uint8_t sensePin, uint8_t adcBits) : adc(0x01 << adcBits)
{
	this->sensePin = sensePin;
	this->activationPin = 0xFF;
	this->minVoltage = minVoltage;
	this->maxVoltage = maxVoltage;
	this->ischarge = 0;
	analogReadResolution(adcBits);
}

void Battery::begin(uint16_t refVoltage, float dividerRatio, mapFn_t mapFunction)
{
	this->refVoltage = refVoltage;
	this->dividerRatio = dividerRatio;
	pinMode(this->sensePin, INPUT);
	analogSetPinAttenuation(this->sensePin, ADC_2_5db); // set attenuation to 2.5db
	this->mapFunction = mapFunction ? mapFunction : &linear;
}

void Battery::onDemand(uint8_t activationPin, uint8_t activationMode)
{
	this->activationPin = activationPin;
	if (activationPin < 0xFF)
	{
		this->activationMode = activationMode;
		pinMode(this->activationPin, OUTPUT);
		digitalWrite(activationPin, !activationMode);
	}
}

uint8_t Battery::level()
{
	return this->level(this->voltage());
}

uint8_t Battery::level(uint16_t voltage)
{
	// Serial.print("当前电压: ");
	// Serial.print(voltage);
	// Serial.print("  maxVoltage: ");
	// Serial.print(maxVoltage);

	if (voltage <= minVoltage)
	{
		return 0;
	}
	else if (voltage >= maxVoltage)
	{
		if (4300 < voltage)
		{
			this->ischarge = true;
		}
		else
		{
			this->ischarge = false;
		}

		return 100;
	}
	else
	{
		return (*mapFunction)(voltage, minVoltage, maxVoltage);
	}
}

uint8_t Battery::getischarge(void)
{
	return this->ischarge;
}

uint16_t Battery::voltage(uint8_t msDelay)
{
	delay(msDelay);

	// 读取当前ADC值（单位：毫伏）
	uint16_t currentReading = analogReadMilliVolts(sensePin);

	// 低通滤波器参数
	// alpha = 0.1 表示新值权重10%，旧值权重90%（强滤波，响应慢但平滑）
	// alpha = 0.3 表示新值权重30%，旧值权重70%（中等滤波，平衡响应和平滑）
	// alpha = 0.5 表示新值权重50%，旧值权重50%（弱滤波，响应快但不够平滑）
	const float alpha = 0.2;  // 滤波系数，可根据需要调整

	// 静态变量保存上次滤波后的值
	static uint16_t filteredValue = 0;
	static bool initialized = false;

	// 首次运行时初始化
	if (!initialized) {
		filteredValue = currentReading;
		initialized = true;
	}

	// 低通滤波公式: filtered = alpha * current + (1 - alpha) * filtered_old
	filteredValue = (uint16_t)(alpha * currentReading + (1.0 - alpha) * filteredValue);

	// 应用分压比（4倍）得到实际电池电压
	uint16_t reading = filteredValue * 4;

	return reading;
}