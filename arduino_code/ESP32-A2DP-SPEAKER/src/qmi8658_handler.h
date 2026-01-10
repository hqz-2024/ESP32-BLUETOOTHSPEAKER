#ifndef QMI8658_HANDLER_H
#define QMI8658_HANDLER_H

#include <Arduino.h>

void initQMI8658Handler();

void setQMI8658EEUIInstance(void* eeui);

void updateQMI8658();

bool isShakeDetected();

void getAcceleration(float* x, float* y, float* z);

void getGyroscope(float* x, float* y, float* z);

#endif

