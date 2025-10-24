/**
 * ESP32 经典蓝牙A2DP音箱程序
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
#define I2S_BCK_PIN     33    // Bit Clock
#define I2S_LRCK_PIN    26    // Left/Right Clock (WS)
#define I2S_DIN_PIN     25    // Data In
#define I2S_MUTE_PIN    27    // Mute Control (可选)
#define BOOT_BUTTON_PIN 0     // BOOT按钮
#define VOLUME_ADC_PIN  34    // 音量控制ADC

// ==================== 全局对象 ====================
BluetoothA2DPSink a2dp_sink;
OneButton bootButton(BOOT_BUTTON_PIN, true);
Preferences preferences;

// ==================== 全局变量 ====================
float currentVolume = 0.8;  // 默认音量80%
unsigned long lastVolumeCheck = 0;
const unsigned long VOLUME_CHECK_INTERVAL = 300;
bool isConnected = false;
String deviceName = "ESP-AI-SPEAKER";
int buttonClickCount = 0;
unsigned long lastClickTime = 0;
const unsigned long MULTI_CLICK_TIMEOUT = 1000;

// ==================== I2S配置 ====================
void setupI2S() {
  // 配置MUTE引脚（可选）
  pinMode(I2S_MUTE_PIN, OUTPUT);
  digitalWrite(I2S_MUTE_PIN, HIGH); // PCM5102 MUTE引脚高电平取消静音

  // I2S配置结构体
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,  // 不使用APLL，PCM5102不需要
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  // I2S引脚配置
  i2s_pin_config_t pin_config = {
    .mck_io_num = I2S_PIN_NO_CHANGE,  // PCM5102不需要MCLK
    .bck_io_num = I2S_BCK_PIN,        // Bit Clock
    .ws_io_num = I2S_LRCK_PIN,        // Word Select (L/R Clock)
    .data_out_num = I2S_DIN_PIN,      // Data Out
    .data_in_num = I2S_PIN_NO_CHANGE  // 不使用输入
  };

  // 安装I2S驱动
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
        audioData[i] = (int16_t)(audioData[i] * currentVolume*0.2);
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
  Serial.printf("A2DP连接状态变化: %s\n", 
    state == ESP_A2D_CONNECTION_STATE_CONNECTED ? "已连接" : "已断开");
  
  isConnected = (state == ESP_A2D_CONNECTION_STATE_CONNECTED);
  
  if (isConnected) {
    Serial.println("蓝牙设备已连接，保存配对信息");
    saveBluetoothConfig();
  } else {
    Serial.println("蓝牙设备已断开，等待重连...");
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
    digitalWrite(I2S_MUTE_PIN, HIGH); 
    Serial.printf("音量调整: %.2f (ADC: %d)\n", currentVolume, adcValue);
  }
  else if (newVolume <= 0.1) {
    digitalWrite(I2S_MUTE_PIN, LOW); 
    Serial.printf("音量调整:静音\n");
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

  // 停止A2DP服务
  a2dp_sink.end();

  // 获取已配对设备数量
  int bond_device_num = esp_bt_gap_get_bond_device_num();
  Serial.printf("发现 %d 个已配对设备\n", bond_device_num);

  if (bond_device_num > 0) {
    // 分配内存存储设备地址列表
    esp_bd_addr_t *bond_device_list = (esp_bd_addr_t *)malloc(sizeof(esp_bd_addr_t) * bond_device_num);
    if (bond_device_list) {
      // 获取已配对设备列表
      esp_err_t ret = esp_bt_gap_get_bond_device_list(&bond_device_num, bond_device_list);
      if (ret == ESP_OK) {
        Serial.println("开始清除配对设备...");
        // 逐个删除配对设备
        for (int i = 0; i < bond_device_num; i++) {
          esp_err_t remove_ret = esp_bt_gap_remove_bond_device(bond_device_list[i]);
          Serial.printf("删除配对设备 %d: %s\n", i + 1, remove_ret == ESP_OK ? "成功" : "失败");
        }
      } else {
        Serial.printf("获取配对设备列表失败: %d\n", ret);
      }
      free(bond_device_list);
    } else {
      Serial.println("内存分配失败，无法获取配对设备列表");
    }
  } else {
    Serial.println("没有发现已配对设备");
  }

  // 清除Preferences配置
  preferences.begin("bluetooth", false);
  preferences.clear();
  preferences.end();
  Serial.println("已清除本地配置信息");

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
  Serial.println("版本：纯A2DP库实现，不使用AudioTools");

  // 配置ADC分辨率
  analogReadResolution(12);

  // 配置按钮
  bootButton.attachClick(handleButtonClick);
  bootButton.setClickTicks(250);
  bootButton.setPressTicks(1000);
  bootButton.setDebounceTicks(50);

  // 配置I2S硬件
  setupI2S();

  // 设置A2DP音频数据回调
  a2dp_sink.set_stream_reader(read_data_stream, false);

  // 启用自动重连功能
  a2dp_sink.set_auto_reconnect(true);

  // 设置连接状态回调
  a2dp_sink.set_on_connection_state_changed(connection_state_changed);
  a2dp_sink.set_on_audio_state_changed(audio_state_changed);

  // 启动A2DP蓝牙接收器
  a2dp_sink.start("ESP-AI-SPEAKER");

  Serial.println("PCM5102音箱已启动");
  Serial.println("蓝牙设备名称: ESP-AI-SPEAKER");
  Serial.println("自动重连已启用");
  Serial.println("等待蓝牙连接...");
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
  
  // 状态指示和重连状态
  static unsigned long lastStatusPrint = 0;
  if (currentTime - lastStatusPrint >= 10000) {
    esp_a2d_connection_state_t conn_state = a2dp_sink.get_connection_state();
    Serial.printf("状态 - 连接: %s, 音量: %.2f, 重连状态: %s\n", 
                  isConnected ? "已连接" : "未连接", 
                  currentVolume,
                  conn_state == ESP_A2D_CONNECTION_STATE_CONNECTING ? "重连中" : "空闲");
    lastStatusPrint = currentTime;
  }
  
  delay(10);
}


