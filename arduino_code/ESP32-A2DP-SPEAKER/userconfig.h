/**
 * ESP32 A2DP音箱硬件配置文件
 * 
 *  * Copyright (c) 2026 Cyberware Workshop
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
 *
 * 此文件包含所有硬件相关的引脚定义和配置参数
 * 修改此文件可以适配不同的硬件平台
 * 
 */

#ifndef USERCONFIG_H
#define USERCONFIG_H

// ==================== I2S音频输出引脚配置 ====================
// PCM5102 DAC模块连接
#define I2S_BCK_PIN     33    // I2S位时钟 (Bit Clock)
#define I2S_LRCK_PIN    26    // I2S左右声道时钟 (Left/Right Clock / WS)
#define I2S_DIN_PIN     25    // I2S数据输入 (Data In)
#define I2S_MUTE_PIN    27    // 静音控制引脚 (可选，PCM5102的MUTE引脚)

// ==================== 控制引脚配置 ====================
#define BOOT_BUTTON_PIN 0     // BOOT按钮引脚（用于恢复出厂设置）
#define VOLUME_ADC_PIN  -1    // 音量控制ADC输入引脚
#define WS2812_PIN      2    // WS2812 RGB LED数据引脚
#define BATTERY_PIN     35    // 电池电压检测引脚

// ==================== I2S音频参数配置 ====================
#define I2S_SAMPLE_RATE     44100                       // 采样率 (Hz)
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT  // 位深度
#define I2S_DMA_BUF_COUNT   8                           // DMA缓冲区数量
#define I2S_DMA_BUF_LEN     512                         // DMA缓冲区长度

// ==================== 音量控制参数 ====================
#define DEFAULT_VOLUME          0.8     // 默认音量 (0.0 - 1.0)
#define VOLUME_CHECK_INTERVAL   300     // 音量检查间隔 (毫秒)
#define VOLUME_QUANTIZE_STEPS   20      // 音量量化档位数 (0-20档)
#define VOLUME_MAX_GAIN         0.6     // 最大音量增益限制

// ==================== 按钮控制参数 ====================
#define MULTI_CLICK_TIMEOUT     1000    // 多击超时时间 (毫秒)
#define FACTORY_RESET_CLICKS    5       // 恢复出厂设置所需点击次数
#define BUTTON_CLICK_TICKS      200     // 按钮点击时长 (毫秒)
#define BUTTON_PRESS_TICKS      1000    // 按钮长按时长 (毫秒)
#define BUTTON_DEBOUNCE_TICKS   100     // 按钮防抖时长 (毫秒) - 增加到100ms防止误触发
#define BUTTON_IDLE_TICKS       150     // 按钮空闲时长 (毫秒) - 两次点击之间的最小间隔

// ==================== LED控制参数 ====================
#define WS2812_LED_COUNT        1       // WS2812 LED数量
#define LED_BRIGHTNESS          100     // LED亮度 (0-255)
#define LED_BLINK_INTERVAL      1000    // LED闪烁间隔 (毫秒)
#define LED_BREATH_INTERVAL     30      // LED呼吸灯更新间隔 (毫秒)
#define LED_BREATH_STEP         3       // LED呼吸灯亮度步进值

// ==================== 蓝牙配置参数 ====================
#define BT_DEVICE_NAME          "ESP-AI-SPEAKER"  // 蓝牙设备名称
#define BT_AUTO_RECONNECT       true              // 是否启用自动重连
#define STATUS_PRINT_INTERVAL   10000             // 状态打印间隔 (毫秒)

// ==================== LED颜色定义 ====================
// RGB颜色值 (R, G, B)
#define LED_COLOR_OFF           0, 0, 0      // 熄灭
#define LED_COLOR_BLUE          0, 0, 255    // 蓝色 - 未连接/已连接未播放
#define LED_COLOR_GREEN         0, 255, 0    // 绿色 - 播放中
#define LED_COLOR_RED           255, 0, 0    // 红色 - 错误状态
#define LED_COLOR_YELLOW        255, 255, 0  // 黄色 - 警告状态

// ==================== PCA9554 配置参数 ====================

// I2C 引脚配置
#define I2C_SDA_PIN 4      // SDA 引脚
#define I2C_SCL_PIN 15     // SCL 引脚
#define I2C_FREQ 100000    // I2C 频率 (100kHz)
#define PCA9554_ADDR 0x38
#define INT_PIN 39

// ==================== 屏幕配置 ====================
#define SCREEN_WIDTH 240 
#define SCREEN_HEIGHT 320
#define SCREEN_PAD_LEFT 8
#define SCREEN_PAD_RIGHT 8

// ==================== QMI8658 配置参数 ====================

#define QMI8658_SDA_PIN     I2C_SDA_PIN // 使用I2C总线
#define QMI8658_SCL_PIN     I2C_SCL_PIN // 使用I2C总线
#define QMI8658_INT_PIN     36    // QMI8658中断引脚
#define QMI8658_ADDR        0x6A  // QMI8658 I2C地址
#define SHAKE_THRESHOLD     2.5   // 触发摇一摇的加速度阈值 (g)
#define SHAKE_COOLDOWN_MS   1000  // 摇一摇冷却时间 (毫秒)

#endif
