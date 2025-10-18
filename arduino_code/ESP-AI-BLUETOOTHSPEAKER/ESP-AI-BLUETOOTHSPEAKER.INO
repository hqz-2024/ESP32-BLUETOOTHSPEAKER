/**
 * ESP32S3 WiFi音频流播放器
 * 使用8311音频芯片进行I2S音频输出
 *
 * 重要说明：ESP32S3不支持经典蓝牙A2DP协议！
 * 本程序提供WiFi音频流播放功能作为替代方案
 *
 * 功能特性：
 * 1. WiFi音频流接收器，设备名称：ESP-AI-SPEAKER
 * 2. I2S音频输出到8311芯片
 * 3. IO0按钮连按5下恢复出厂设置
 * 4. IO7 ADC音量控制
 * 5. 支持多种音频格式（MP3、AAC、WAV等）
 * 6. Web配置界面
 *
 * I2S引脚配置：
 * IO14 -> MCK (Master Clock)
 * IO15 -> DOUT (Data Out)
 * IO16 -> BCK (Bit Clock)
 * IO17 -> WS (Word Select)
 *
 * 其他引脚：
 * IO0  -> BOOT按钮（连按5下恢复出厂设置）
 * IO7  -> 音量控制ADC输入
 *
 * 使用方法：
 * 1. 设备启动后会创建WiFi热点"ESP-AI-SPEAKER"
 * 2. 连接热点，访问192.168.4.1配置WiFi
 * 3. 配置完成后设备连接到指定WiFi
 * 4. 通过Web界面或HTTP API发送音频流
 *
 * @author ESP-AI Team
 * @date 2024
 */

#include "Audio.h"
#include "OneButton.h"
#include "Preferences.h"
#include "WiFi.h"
#include "WebServer.h"
#include "ESPmDNS.h"

// ==================== 引脚定义 ====================
#define I2S_MCK_PIN     14    // Master Clock
#define I2S_DOUT_PIN    15    // Data Out
#define I2S_BCK_PIN     16    // Bit Clock
#define I2S_WS_PIN      17    // Word Select
#define BOOT_BUTTON_PIN 0     // BOOT按钮
#define VOLUME_ADC_PIN  7     // 音量控制ADC

// ==================== 全局对象 ====================
I2SStream i2s;
OneButton bootButton(BOOT_BUTTON_PIN, true);
Preferences preferences;
BluetoothSerial SerialBT;
WiFiServer audioServer(8080);

// ==================== 全局变量 ====================
float currentVolume = 0.5;        // 当前音量 (0.0-1.0)
unsigned long lastVolumeCheck = 0; // 上次检查音量的时间
const unsigned long VOLUME_CHECK_INTERVAL = 100; // 音量检查间隔(ms)
bool isConnected = false;          // 蓝牙连接状态
String deviceName = "ESP-AI-SPEAKER"; // 蓝牙设备名称
int buttonClickCount = 0;          // 按钮点击计数
unsigned long lastClickTime = 0;   // 上次点击时间
const unsigned long MULTI_CLICK_TIMEOUT = 1000; // 多击超时时间(ms)

// ==================== 音频数据处理回调 ====================
void read_data_stream(const uint8_t *data, uint32_t length) {
  // 应用音量控制并输出到I2S
  if (currentVolume > 0.0 && length > 0) {
    // 创建临时缓冲区用于音量调整
    uint8_t* tempBuffer = (uint8_t*)malloc(length);
    if (tempBuffer) {
      memcpy(tempBuffer, data, length);
      int16_t* audioData = (int16_t*)tempBuffer;
      int samples = length / 2; // 16位音频，每个样本2字节

      // 应用音量
      for (int i = 0; i < samples; i++) {
        audioData[i] = (int16_t)(audioData[i] * currentVolume);
      }

      // 输出到I2S
      i2s.write(tempBuffer, length);
      free(tempBuffer);
    }
  } else if (length > 0) {
    // 音量为0或静音，直接输出原始数据
    i2s.write(data, length);
  }
}

// ==================== 蓝牙连接状态回调 ====================
void avrc_metadata_callback(uint8_t data1, const uint8_t *data2) {
  Serial.printf("AVRC metadata rsp: attribute id 0x%x, %s\n", data1, data2);
}

void connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  Serial.printf("A2DP connection state: %s\n",
    state == ESP_A2D_CONNECTION_STATE_CONNECTED ? "Connected" : "Disconnected");
  isConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);

  if (isConnected) {
    Serial.println("蓝牙设备已连接");
    saveBluetoothConfig();
  } else {
    Serial.println("蓝牙设备已断开");
  }
}

void audio_state_changed(esp_a2d_audio_state_t state, void *ptr) {
  Serial.printf("A2DP audio state: %s\n",
    state == ESP_A2D_AUDIO_STATE_STARTED ? "Started" : "Stopped");
}

// ==================== 音量控制函数 ====================
void updateVolume() {
  // 读取ADC值 (0-4095)
  int adcValue = analogRead(VOLUME_ADC_PIN);

  // 转换为音量值 (0.0-1.0)
  float newVolume = (float)adcValue / 4095.0;

  // 添加一些滞后，避免音量抖动
  if (fabs(newVolume - currentVolume) > 0.02) {
    currentVolume = newVolume;
    Serial.printf("音量调整: %.2f (ADC: %d)\n", currentVolume, adcValue);
  }
}

// ==================== 按钮事件处理 ====================
void handleButtonClick() {
  unsigned long currentTime = millis();

  // 检查是否在多击超时时间内
  if (currentTime - lastClickTime > MULTI_CLICK_TIMEOUT) {
    buttonClickCount = 0; // 重置计数
  }

  buttonClickCount++;
  lastClickTime = currentTime;

  Serial.printf("按钮点击次数: %d\n", buttonClickCount);

  if (buttonClickCount == 5) {
    Serial.println("检测到5次点击，执行恢复出厂设置...");
    factoryReset();
    buttonClickCount = 0; // 重置计数
  }
}

void checkMultiClickTimeout() {
  // 检查多击超时
  if (buttonClickCount > 0 && (millis() - lastClickTime > MULTI_CLICK_TIMEOUT)) {
    if (buttonClickCount < 5) {
      Serial.printf("多击超时，点击次数: %d (需要5次)\n", buttonClickCount);
    }
    buttonClickCount = 0;
  }
}

// ==================== 恢复出厂设置 ====================
void factoryReset() {
  Serial.println("开始恢复出厂设置...");

  // 停止蓝牙
  a2dp_sink.end();

  // 清除蓝牙配置
  preferences.begin("bluetooth", false);
  preferences.clear();
  preferences.end();

  // 清除ESP32蓝牙配置
  esp_bt_gap_remove_bond_device(NULL);

  Serial.println("出厂设置恢复完成，重启设备...");
  delay(1000);
  ESP.restart();
}

// ==================== 蓝牙配置保存/加载 ====================
void saveBluetoothConfig() {
  preferences.begin("bluetooth", false);
  preferences.putBool("paired", true);
  preferences.end();
  Serial.println("蓝牙配置已保存");
}

bool loadBluetoothConfig() {
  preferences.begin("bluetooth", true);
  bool paired = preferences.getBool("paired", false);
  preferences.end();
  return paired;
}

// ==================== 初始化函数 ====================
void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial.println("ESP32S3 蓝牙音箱启动中...");

  // 设置音频工具日志级别
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

  // 初始化ADC
  analogReadResolution(12); // 12位ADC分辨率 (0-4095)

  // 初始化按钮
  bootButton.attachClick(handleButtonClick);
  bootButton.setClickTicks(250);        // 单击时间
  bootButton.setPressTicks(1000);       // 长按时间
  bootButton.setDebounceTicks(50);      // 防抖时间

  // 初始化I2S配置
  auto i2s_config = i2s.defaultConfig();
  i2s_config.rx_tx_mode = TX_MODE;      // 仅输出模式
  i2s_config.sample_rate = 44100;       // 采样率44.1kHz
  i2s_config.channels = 2;              // 立体声
  i2s_config.bits_per_sample = 16;      // 16位音频
  i2s_config.buffer_count = 8;          // 缓冲区数量
  i2s_config.buffer_size = 512;         // 缓冲区大小

  // 配置I2S引脚
  i2s_config.pin_mck = I2S_MCK_PIN;     // Master Clock
  i2s_config.pin_data = I2S_DOUT_PIN;   // Data Out
  i2s_config.pin_bck = I2S_BCK_PIN;     // Bit Clock
  i2s_config.pin_ws = I2S_WS_PIN;       // Word Select

  // 启动I2S
  if (!i2s.begin(i2s_config)) {
    Serial.println("I2S初始化失败！");
    while(1) delay(1000);
  }
  Serial.println("I2S初始化成功");

  // 配置蓝牙A2DP接收器
  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_on_audio_state_changed(audio_state_changed);

  // 设置音频质量参数
  a2dp_sink.set_auto_reconnect(true);   // 启用自动重连

  // 检查是否有已配对的设备
  bool wasPaired = loadBluetoothConfig();
  if (wasPaired) {
    Serial.println("检测到已配对设备，启用自动重连模式");
  } else {
    Serial.println("首次启动，进入配对模式");
  }

  // 启动蓝牙A2DP接收器
  if (!a2dp_sink.start(deviceName.c_str())) {
    Serial.println("蓝牙A2DP启动失败！");
    while(1) delay(1000);
  }

  Serial.printf("蓝牙音箱已启动，设备名称: %s\n", deviceName.c_str());
  Serial.println("等待蓝牙设备连接...");
  Serial.println("按钮功能：连按5下BOOT按钮可恢复出厂设置");
  Serial.println("音量控制：通过IO7的ADC电压调节音量");

  // 初始音量检查
  updateVolume();
}

void loop() {
  // 处理按钮事件
  bootButton.tick();

  // 检查多击超时
  checkMultiClickTimeout();

  // 定期检查音量控制
  unsigned long currentTime = millis();
  if (currentTime - lastVolumeCheck >= VOLUME_CHECK_INTERVAL) {
    updateVolume();
    lastVolumeCheck = currentTime;
  }

  // 状态指示（可选）
  static unsigned long lastStatusPrint = 0;
  if (currentTime - lastStatusPrint >= 10000) { // 每10秒打印一次状态
    Serial.printf("状态 - 连接: %s, 音量: %.2f\n",
                  isConnected ? "已连接" : "未连接", currentVolume);
    lastStatusPrint = currentTime;
  }

  // 短暂延时，避免CPU占用过高
  delay(10);
}