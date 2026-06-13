/** Copyright (c) 2026 Cyberware Workshop
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Commercial use of this software requires prior written authorization from the Licensor.
 * 请注意：将 Cyberware Workshop 代码用于商业用途需要事先获得许可方的授权。
 * 删除与修改版权属于侵权行为，请尊重作者版权，避免产生不必要的纠纷。
 *
 * @author Cyberware Workshop Team
 * @date 2026
 */

#include "qmi8658_handler.h"
#include "album_cover_manager.h"
#include "bluetooth_manager.h"
#include "eeui.h"
#include "../userconfig.h"
#include <QMI8658A.h>
#include <Wire.h>

extern TwoWire Wire;

static QMI8658A imu;
static bool imu_initialized = false;
static unsigned long last_shake_time = 0;
static EEUI* g_eeui = nullptr;

static void IRAM_ATTR qmi8658_isr() {}

void setQMI8658EEUIInstance(void* eeui) {
    g_eeui = (EEUI*)eeui;
}

void initQMI8658Handler() {
    Wire.beginTransmission(QMI8658_ADDR);
    uint8_t error = Wire.endTransmission();

    if (error != 0) {
        imu_initialized = false;
        return;
    }

    imu.begin(QMI8658_ADDR, 400000);

    imu.setAccScale(acc_scale_4g);
    imu.setGyroScale(gyro_scale_1024dps);
    imu.setAccODR(acc_odr_norm_500);
    imu.setGyroODR(gyro_odr_norm_500);
    imu.setAccLPF(lpf_13_37);
    imu.setGyroLPF(lpf_13_37);
    imu.setState(sensor_running);

    delay(1000);

    const int MAX_RETRY = 5;
    bool init_success = false;

    for (int retry = 0; retry < MAX_RETRY; retry++) {
        if (retry > 0) {
            delay(1000);
        }

        imu.getAccX();
        imu.getAccY();
        imu.getAccZ();
        delay(100);

        float x1 = imu.getAccX();
        float y1 = imu.getAccY();
        float z1 = imu.getAccZ();
        float mag1 = sqrt(x1*x1 + y1*y1 + z1*z1);
        delay(50);

        float x2 = imu.getAccX();
        float y2 = imu.getAccY();
        float z2 = imu.getAccZ();
        float mag2 = sqrt(x2*x2 + y2*y2 + z2*z2);
        delay(50);

        float x3 = imu.getAccX();
        float y3 = imu.getAccY();
        float z3 = imu.getAccZ();
        float mag3 = sqrt(x3*x3 + y3*y3 + z3*z3);

        float avg_mag = (mag1 + mag2 + mag3) / 3.0;
        float diff = max(abs(mag1 - mag2), max(abs(mag2 - mag3), abs(mag1 - mag3)));

        if (avg_mag > 0.8 && avg_mag < 1.2 && diff < 0.2) {
            init_success = true;
            break;
        }
    }

    if (init_success) {
        imu_initialized = true;
        pinMode(QMI8658_INT_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(QMI8658_INT_PIN), qmi8658_isr, FALLING);
    } else {
        imu_initialized = false;
    }
}

void updateQMI8658() {
    if (!imu_initialized) return;

    static unsigned long last_check_time = 0;
    if (millis() - last_check_time < 300) {
        return;
    }
    last_check_time = millis();

    if (millis() - last_shake_time < SHAKE_COOLDOWN_MS) {
        return;
    }

    float ax = imu.getAccX();
    float ay = imu.getAccY();
    float az = imu.getAccZ();

    float magnitude = sqrt(ax*ax + ay*ay + az*az);

    if (magnitude > SHAKE_THRESHOLD) {
        if (!g_eeui) {
            return;
        }

        const lv_img_dsc_t* newCover = nextAlbumCover();
        bool currentPlaying = isAudioPlaying();
        g_eeui->render_rotating_image(newCover, currentPlaying);

        last_shake_time = millis();
    }
}

void getAcceleration(float* x, float* y, float* z) {
    if (!imu_initialized) {
        *x = *y = *z = 0;
        return;
    }
    *x = imu.getAccX();
    *y = imu.getAccY();
    *z = imu.getAccZ();
}

void getGyroscope(float* x, float* y, float* z) {
    if (!imu_initialized) {
        *x = *y = *z = 0;
        return;
    }
    *x = imu.getGyroX();
    *y = imu.getGyroY();
    *z = imu.getGyroZ();
}
