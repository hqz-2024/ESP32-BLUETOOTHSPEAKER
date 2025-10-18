/**
 * ESP32S3 蓝牙音箱程序 - 简化版本
 * 使用8311音频芯片进行I2S音频输出
 * 
 * 如果完整版本有库兼容性问题，可以使用此简化版本
 * 
 * @author ESP-AI Team
 * @date 2024
 */

#include "AudioTools.h"
#include "OneButton.h"
#include "Preferences.h"

// 尝试包含A2DP库，如果失败则使用备用方案
#ifdef ESP32
  #include "BluetoothA2DPSink.h"
  #define USE_A2DP_LIB
#endif

// ==================== 引脚定义 ====================
#define I2S_MCK_PIN     14    // Master Clock
#define I2S_DOUT_PIN    15    // Data Out
#define I2S_BCK_PIN     16    // Bit Clock
#define I2S_WS_PIN      17    // Word Select
#define BOOT_BUTTON_PIN 0     // BOOT按钮
#define VOLUME_ADC_PIN  7     // 音量控制ADC

// ==================== 全局对象 ====================
#ifdef USE_A2DP_LIB
BluetoothA2DPSink a2dp_sink;
#endif
I2SStream i2s;
OneButton bootButton(BOOT_BUTTON_PIN, true);
Preferences preferences;

// ==================== 全局变量 ====================
float currentVolume = 0.5;
unsigned long lastVolumeCheck = 0;
const unsigned long VOLUME_CHECK_INTERVAL = 100;
bool isConnected = false;
String deviceName = "ESP-AI-SPEAKER";
int buttonClickCount = 0;
unsigned long lastClickTime = 0;
const unsigned long MULTI_CLICK_TIMEOUT = 1000;

// ==================== 音频数据处理回调 ====================
#ifdef USE_A2DP_LIB
void read_data_stream(const uint8_t *data, uint32_t length) {
  if (currentVolume > 0.0 && length > 0) {
    uint8_t* tempBuffer = (uint8_t*)malloc(length);
    if (tempBuffer) {
      memcpy(tempBuffer, data, length);
      int16_t* audioData = (int16_t*)tempBuffer;
      int samples = length / 2;
      
      for (int i = 0; i < samples; i++) {
        audioData[i] = (int16_t)(audioData[i] * currentVolume);
      }
      
      i2s.write(tempBuffer, length);
      free(tempBuffer);
    }
  } else if (length > 0) {
    i2s.write(data, length);
  }
}

void connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  isConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
  Serial.printf("蓝牙连接状态: %s\n", isConnected ? "已连接" : "已断开");
}
#endif

// ==================== 音量控制函数 ====================
void updateVolume() {
  int adcValue = analogRead(VOLUME_ADC_PIN);
  float newVolume = (float)adcValue / 4095.0;
  
  if (fabs(newVolume - currentVolume) > 0.02) {
    currentVolume = newVolume;
    Serial.printf("音量调整: %.2f (ADC: %d)\n", currentVolume, adcValue);
  }
}

// ==================== 按钮事件处理 ====================
void handleButtonClick() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastClickTime > MULTI_CLICK_TIMEOUT) {
    buttonClickCount = 0;
  }
  
  buttonClickCount++;
  lastClickTime = currentTime;
  
  Serial.printf("按钮点击次数: %d\n", buttonClickCount);
  
  if (buttonClickCount == 5) {
    Serial.println("检测到5次点击，执行恢复出厂设置...");
    factoryReset();
    buttonClickCount = 0;
  }
}

void checkMultiClickTimeout() {
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
  
#ifdef USE_A2DP_LIB
  a2dp_sink.end();
#endif
  
  preferences.begin("bluetooth", false);
  preferences.clear();
  preferences.end();
  
  Serial.println("出厂设置恢复完成，重启设备...");
  delay(1000);
  ESP.restart();
}

// ==================== 蓝牙配置保存/加载 ====================
void saveBluetoothConfig() {
  preferences.begin("bluetooth", false);
  preferences.putBool("paired", true);
  preferences.end();
}

bool loadBluetoothConfig() {
  preferences.begin("bluetooth", true);
  bool paired = preferences.getBool("paired", false);
  preferences.end();
  return paired;
}

// ==================== 初始化函数 ====================
void setup() {
  Serial.begin(115200);
  Serial.println("ESP32S3 蓝牙音箱启动中...");
  
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);
  
  analogReadResolution(12);
  
  bootButton.attachClick(handleButtonClick);
  bootButton.setClickTicks(250);
  bootButton.setPressTicks(1000);
  bootButton.setDebounceTicks(50);
  
  // 初始化I2S
  auto i2s_config = i2s.defaultConfig();
  i2s_config.rx_tx_mode = TX_MODE;
  i2s_config.sample_rate = 44100;
  i2s_config.channels = 2;
  i2s_config.bits_per_sample = 16;
  i2s_config.buffer_count = 8;
  i2s_config.buffer_size = 512;
  
  i2s_config.pin_mck = I2S_MCK_PIN;
  i2s_config.pin_data = I2S_DOUT_PIN;
  i2s_config.pin_bck = I2S_BCK_PIN;
  i2s_config.pin_ws = I2S_WS_PIN;
  
  if (!i2s.begin(i2s_config)) {
    Serial.println("I2S初始化失败！");
    while(1) delay(1000);
  }
  Serial.println("I2S初始化成功");
  
#ifdef USE_A2DP_LIB
  // 配置蓝牙A2DP
  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_auto_reconnect(true);
  
  if (!a2dp_sink.start(deviceName.c_str())) {
    Serial.println("蓝牙A2DP启动失败！");
    while(1) delay(1000);
  }
  
  Serial.printf("蓝牙音箱已启动，设备名称: %s\n", deviceName.c_str());
#else
  Serial.println("警告：未找到A2DP库，仅测试I2S和按钮功能");
#endif
  
  updateVolume();
}

void loop() {
  bootButton.tick();
  checkMultiClickTimeout();
  
  unsigned long currentTime = millis();
  if (currentTime - lastVolumeCheck >= VOLUME_CHECK_INTERVAL) {
    updateVolume();
    lastVolumeCheck = currentTime;
  }
  
  static unsigned long lastStatusPrint = 0;
  if (currentTime - lastStatusPrint >= 10000) {
    Serial.printf("状态 - 连接: %s, 音量: %.2f\n", 
                  isConnected ? "已连接" : "未连接", currentVolume);
    lastStatusPrint = currentTime;
  }
  
  delay(10);
}
