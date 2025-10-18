/**
 * ESP32 经典蓝牙A2DP音箱程序
 * 使用8311音频芯片进行I2S音频输出
 * 
 * 重要说明：此程序仅适用于ESP32经典版本，不支持ESP32S3！
 *
 * 功能特性：
 * 1. 蓝牙A2DP接收器，设备名称：ESP-AI-SPEAKER
 * 2. I2S音频输出到8311芯片
 * 3. IO0按钮连按5下恢复出厂设置
 * 4. IO7 ADC音量控制
 * 5. 自动重连已配对设备
 * 6. 低延时高音质音频传输
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
 * 1. 首次开机进入蓝牙广播模式
 * 2. 手机搜索"ESP-AI-SPEAKER"并连接
 * 3. 连接成功后自动播放手机音频
 * 4. 支持所有标准A2DP设备
 *
 * @author ESP-AI Team
 * @date 2024
 */

#include "BluetoothA2DPSink.h"
#include "OneButton.h"
#include "Preferences.h"
#include "driver/i2s.h"

// ==================== 引脚定义 ====================
#define I2S_SCK_PIN     32    // System Clock (MCLK)
#define I2S_DIN_PIN     25    // Data In
#define I2S_BCK_PIN     33    // Bit Clock
#define I2S_LRCK_PIN    26    // Left/Right Clock (WS)
#define I2S_MUTE_PIN    27    // Mute Control
#define BOOT_BUTTON_PIN 0     // BOOT按钮
#define VOLUME_ADC_PIN  34     // 音量控制ADC

// ==================== 全局对象 ====================
BluetoothA2DPSink a2dp_sink;
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

// ==================== I2S配置 ====================
void setupI2S() {
  // 配置MUTE引脚
  pinMode(I2S_MUTE_PIN, OUTPUT);
  digitalWrite(I2S_MUTE_PIN, HIGH); // PCM5102 MUTE引脚高电平取消静音
  
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = true,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .mck_io_num = I2S_SCK_PIN,      // Master Clock
    .bck_io_num = I2S_BCK_PIN,      // Bit Clock
    .ws_io_num = I2S_LRCK_PIN,      // Word Select (L/R Clock)
    .data_out_num = I2S_DIN_PIN,    // Data Out
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
  
  Serial.println("PCM5102 I2S配置完成");
}

// ==================== 音频数据处理回调 ====================
void read_data_stream(const uint8_t *data, uint32_t length) {
  if (currentVolume > 0.0 && length > 0) {
    // 创建临时缓冲区用于音量调整
    uint8_t* tempBuffer = (uint8_t*)malloc(length);
    if (tempBuffer) {
      memcpy(tempBuffer, data, length);
      int16_t* audioData = (int16_t*)tempBuffer;
      int samples = length / 2;
      
      // 应用音量控制
      for (int i = 0; i < samples; i++) {
        audioData[i] = (int16_t)(audioData[i] * currentVolume);
      }
      
      // 输出到I2S
      size_t bytes_written;
      i2s_write(I2S_NUM_0, tempBuffer, length, &bytes_written, portMAX_DELAY);
      free(tempBuffer);
    }
  } else if (length > 0) {
    // 音量为0时输出静音
    size_t bytes_written;
    i2s_write(I2S_NUM_0, data, length, &bytes_written, portMAX_DELAY);
  }
}

// ==================== 蓝牙连接状态回调 ====================
void avrc_metadata_callback(uint8_t data1, const uint8_t *data2) {
  Serial.printf("AVRC metadata: attribute id 0x%x, %s\n", data1, data2);
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
  
  a2dp_sink.end();
  
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
  Serial.begin(115200);
  Serial.println("ESP32 蓝牙A2DP音箱启动中...");
  
  analogReadResolution(12);
  
  bootButton.attachClick(handleButtonClick);
  bootButton.setClickTicks(250);
  bootButton.setPressTicks(1000);
  bootButton.setDebounceTicks(50);
  
  // 初始化I2S
  setupI2S();
  Serial.println("I2S初始化成功");
  
  // 配置蓝牙A2DP接收器
  a2dp_sink.set_stream_reader(read_data_stream, false);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_on_audio_state_changed(audio_state_changed);
  
  // 设置音频质量参数
  a2dp_sink.set_auto_reconnect(true);
  
  // 检查是否有已配对的设备
  bool wasPaired = loadBluetoothConfig();
  if (wasPaired) {
    Serial.println("检测到已配对设备，启用自动重连模式");
  } else {
    Serial.println("首次启动，进入配对模式");
  }
  
  // 启动蓝牙A2DP接收器
  a2dp_sink.start(deviceName.c_str());
  Serial.println("蓝牙A2DP已启动");
  
  Serial.printf("蓝牙音箱已启动，设备名称: %s\n", deviceName.c_str());
  Serial.println("等待蓝牙设备连接...");
  Serial.println("按钮功能：连按5下BOOT按钮可恢复出厂设置");
  Serial.println("音量控制：通过IO7的ADC电压调节音量");
  
  updateVolume();
}

void loop() {
  bootButton.tick();
  checkMultiClickTimeout();
  
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
  
  delay(10);
}


