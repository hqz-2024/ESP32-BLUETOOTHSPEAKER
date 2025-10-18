/**
 * ESP32S3 BLE音频接收器
 * 使用8311音频芯片进行I2S音频输出
 * 
 * 重要说明：ESP32S3不支持经典蓝牙A2DP，使用BLE音频方案
 *
 * 功能特性：
 * 1. BLE音频接收器，设备名称：ESP-AI-SPEAKER
 * 2. I2S音频输出到8311芯片
 * 3. IO0按钮连按5下恢复出厂设置
 * 4. IO7 ADC音量控制
 * 5. 蓝牙串口音频数据传输
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
 * 1. 手机需要支持BLE音频或使用专用APP
 * 2. 搜索"ESP-AI-SPEAKER"并连接
 * 3. 通过蓝牙串口发送音频数据
 *
 * @author ESP-AI Team
 * @date 2024
 */

#include "AudioTools.h"
#include "OneButton.h"
#include "Preferences.h"
#include "BluetoothSerial.h"

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

// ==================== 全局变量 ====================
float currentVolume = 0.5;
unsigned long lastVolumeCheck = 0;
const unsigned long VOLUME_CHECK_INTERVAL = 100;
bool isConnected = false;
String deviceName = "ESP-AI-SPEAKER";
int buttonClickCount = 0;
unsigned long lastClickTime = 0;
const unsigned long MULTI_CLICK_TIMEOUT = 1000;

// 音频缓冲区
const int AUDIO_BUFFER_SIZE = 1024;
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
int bufferIndex = 0;

// ==================== 音频数据处理 ====================
void processAudioData() {
  if (SerialBT.available()) {
    while (SerialBT.available() && bufferIndex < AUDIO_BUFFER_SIZE) {
      audioBuffer[bufferIndex++] = SerialBT.read();
    }
    
    // 当缓冲区满时，处理音频数据
    if (bufferIndex >= AUDIO_BUFFER_SIZE) {
      // 应用音量控制
      if (currentVolume != 1.0) {
        int16_t* audioData = (int16_t*)audioBuffer;
        int samples = AUDIO_BUFFER_SIZE / 2;
        
        for (int i = 0; i < samples; i++) {
          audioData[i] = (int16_t)(audioData[i] * currentVolume);
        }
      }
      
      // 输出到I2S
      i2s.write(audioBuffer, AUDIO_BUFFER_SIZE);
      bufferIndex = 0;
    }
  }
}

// ==================== 蓝牙连接回调 ====================
void bluetoothCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("蓝牙客户端已连接");
    isConnected = true;
    saveBluetoothConfig();
  } else if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("蓝牙客户端已断开");
    isConnected = false;
  }
}

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
  
  SerialBT.end();
  
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
  Serial.println("ESP32S3 BLE音频接收器启动中...");
  Serial.println("警告：ESP32S3不支持经典蓝牙A2DP！");
  Serial.println("本程序使用蓝牙串口接收音频数据");
  
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
  
  // 初始化蓝牙串口
  SerialBT.register_callback(bluetoothCallback);
  if (!SerialBT.begin(deviceName)) {
    Serial.println("蓝牙串口初始化失败！");
    while(1) delay(1000);
  }
  
  Serial.printf("BLE音频接收器已启动，设备名称: %s\n", deviceName.c_str());
  Serial.println("等待蓝牙设备连接...");
  Serial.println("注意：需要专用APP发送PCM音频数据");
  Serial.println("音频格式：44.1kHz, 16位, 立体声");
  Serial.println("按钮功能：连按5下BOOT按钮可恢复出厂设置");
  Serial.println("音量控制：通过IO7的ADC电压调节音量");
  
  updateVolume();
}

void loop() {
  bootButton.tick();
  checkMultiClickTimeout();
  
  // 处理音频数据
  processAudioData();
  
  // 定期检查音量控制
  unsigned long currentTime = millis();
  if (currentTime - lastVolumeCheck >= VOLUME_CHECK_INTERVAL) {
    updateVolume();
    lastVolumeCheck = currentTime;
  }
  
  // 状态指示
  static unsigned long lastStatusPrint = 0;
  if (currentTime - lastStatusPrint >= 10000) {
    Serial.printf("状态 - 连接: %s, 音量: %.2f\n", 
                  isConnected ? "已连接" : "未连接", currentVolume);
    lastStatusPrint = currentTime;
  }
  
  delay(1);
}
