/**
 * ESP32 经典蓝牙A2DP音箱程序 - 模块化版本
 *
 * 重要说明：此程序仅适用于ESP32经典版本，不支持ESP32S3！
 *
 * 功能特性：
 * 1. 蓝牙A2DP接收器，设备名称：ESP-AI-SPEAKER
 * 2. I2S音频输出到PCM5102芯片
 * 3. IO0按钮连按5下恢复出厂设置
 * 4. IO34 ADC音量控制
 * 5. 自动重连已配对设备
 * 6. 低延时高音质音频传输
 * 7. WS2812 RGB LED状态指示
 *    - 未连接：蓝色闪烁（1秒间隔）
 *    - 已连接未播放：蓝色长亮
 *    - 播放中：绿色呼吸灯效果
 *
 * 硬件连接：
 * PCM5102 DAC模块：
 *   - BCK  -> GPIO33 (I2S位时钟)
 *   - LRCK -> GPIO26 (I2S左右声道时钟)
 *   - DIN  -> GPIO25 (I2S数据输入)
 *   - GND  -> GND
 *   - VIN  -> 3.3V
 *   - SCK  -> GND (不使用主时钟)
 *   - MUTE -> GPIO27 (静音控制，可选)
 *
 * 其他引脚：
 *   - IO0  -> BOOT按钮（连按5下恢复出厂设置）
 *   - IO34 -> 音量控制ADC输入（可选）
 *   - IO12 -> WS2812 RGB LED数据引脚
 *
 * 使用方法：
 * 1. 首次开机进入蓝牙广播模式（蓝色闪烁）
 * 2. 手机搜索"ESP-AI-SPEAKER"并连接
 * 3. 连接成功后LED变为蓝色长亮
 * 4. 播放音乐时LED显示绿色呼吸灯效果
 * 5. 支持所有标准A2DP设备
 *
 * 模块说明：
 * - src/userconfig.h      - 硬件配置和引脚定义
 * - src/audio_i2s.*       - I2S音频处理模块
 * - src/bluetooth_manager.* - 蓝牙管理模块
 * - src/volume_control.*  - 音量控制模块
 * - src/led_control.*     - LED控制模块
 * - src/button_handler.*  - 按钮处理模块
 * - src/config_manager.*  - 配置管理模块
 *
 * @author ESP-AI Team
 * @date 2024
 */

// 包含所有功能模块
#include "userconfig.h"
#include "src/audio_i2s.h"
#include "src/bluetooth_manager.h"
#include "src/volume_control.h"
#include "src/led_control.h"
#include "src/button_handler.h"
#include "src/config_manager.h"
#include "src/pca9554_handler.h"

// ==================== 初始化函数 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 蓝牙A2DP音箱启动中...");
  Serial.println("版本：模块化架构，易于维护和扩展");
  Serial.println("========================================");
  
  setI2Smute(true);       //先静音，避免开机爆音

  // 初始化各个功能模块
  initVolumeControl();    // 初始化音量控制
  initLedControl();       // 初始化LED控制
  initButtonHandler();    // 初始化按钮处理

  // 初始化蓝牙A2DP（会自动配置I2S）
  initBluetooth(BT_DEVICE_NAME);


  setI2Smute(false);       //配置完成取消静音

  // 设置A2DP音频数据回调
  getA2DPSink()->set_stream_reader(read_data_stream, false);

  // 初始化PCA9554 IO扩展芯片
  if (initPCA9554Handler()) {
    Serial.println("PCA9554 初始化成功");
  } else {
    Serial.println("PCA9554 初始化失败，跳过IO扩展功能");
  }

  Serial.println("========================================");
  Serial.println("PCM5102音箱已启动");
  Serial.printf("蓝牙设备名称: %s\n", BT_DEVICE_NAME);
  Serial.println("等待蓝牙连接...");
}

// ==================== 主循环 ====================
void loop() {
  // 更新按钮状态
  updateButton();
  checkMultiClickTimeout();

  // 更新音量控制
  updateVolume();

  // 更新LED状态指示
  updateRgbLed(isBluetoothConnected(), isAudioPlaying());

  // 更新PCA9554状态
  updatePCA9554();

  // 定期打印状态信息
  static unsigned long lastStatusPrint = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
    esp_a2d_connection_state_t conn_state = getA2DPSink()->get_connection_state();
    Serial.printf("状态 - 连接: %s, 播放: %s, 音量: %.2f, 重连状态: %s\n",
                  isBluetoothConnected() ? "已连接" : "未连接",
                  isAudioPlaying() ? "播放中" : "暂停",
                  getCurrentVolume(),
                  conn_state == ESP_A2D_CONNECTION_STATE_CONNECTING ? "重连中" : "空闲");
    lastStatusPrint = currentTime;
  }

  delay(1);
}


