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

	// 多次采样
	const int samples = 20;
	uint32_t sum = 0;
	uint16_t values[samples];
	uint16_t maxVal = 0;
	uint16_t minVal = 0xFFFF;

	// 采样并记录最值
	// for (int i = 0; i < samples; i++)
	// {
	// 	values[i] = analogReadMilliVolts(sensePin);
	// 	if (values[i] > maxVal)
	// 		maxVal = values[i];
	// 	if (values[i] < minVal)
	// 		minVal = values[i];
	// 	sum += values[i];
	// 	delay(2);
	// }
	
	// 去除最值后的平均
	// sum = sum - maxVal - minVal;
	sum = analogReadMilliVolts(sensePin);
	// uint16_t average = sum / (samples - 2);
	uint16_t average =sum ;
	// 将新值加入滑动窗口
	// voltageWindow[windowIndex] = average;
	// windowIndex = (windowIndex + 1) % WINDOW_SIZE;
	// if (windowIndex == 0)
	// 	windowFull = true;

	// // 计算滑动窗口平均值
	// uint32_t windowSum = 0;
	// int validSamples = windowFull ? WINDOW_SIZE : windowIndex;
	// for (int i = 0; i < validSamples; i++)
	// {
	// 	windowSum += voltageWindow[i];
	// }
	// uint16_t windowAverage = windowSum / validSamples;

	// 最终结果
	// uint16_t reading = windowAverage * 4;
	uint16_t reading = average * 4;
	return reading;
}