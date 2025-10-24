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

 #include "AudioTools.h"
 #include "BluetoothA2DPSink.h"
 #include "OneButton.h"
 #include "Preferences.h"
 #include "driver/i2s.h"
 #include "config.h"

 
 // ==================== 引脚定义 ====================
 #define BOOT_BUTTON_PIN 0     // BOOT按钮
 #define VOLUME_ADC_PIN  34     // 音量控制ADC
 
 // ==================== 全局对象 ====================
 I2SStream i2s;
 BluetoothA2DPSink a2dp_sink(i2s);
 OneButton bootButton(BOOT_BUTTON_PIN, true);
 Preferences preferences;
 
 // ==================== 全局变量 ====================

 unsigned long lastVolumeCheck = 0;
 const unsigned long VOLUME_CHECK_INTERVAL = 100;
 bool isConnected = false;
 String deviceName = "ESP-AI-SPEAKER";
 int buttonClickCount = 0;
 unsigned long lastClickTime = 0;
 const unsigned long MULTI_CLICK_TIMEOUT = 1000;
 
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
   
   analogReadResolution(12);
   
   bootButton.attachClick(handleButtonClick);
   bootButton.setClickTicks(250);
   bootButton.setPressTicks(1000);
   bootButton.setDebounceTicks(50);
   
   // 配置MUTE引脚
   EnableVolume();
   
   // 配置I2S for PCM5102
   auto cfg = i2s.defaultConfig(TX_MODE);
   cfg.sample_rate = 44100;
   cfg.bits_per_sample = 16;
   cfg.channels = 2;
   cfg.pin_bck = 33;        // BCK
   cfg.pin_ws = 26;         // LRCK  
   cfg.pin_data = 25;       // DIN
   cfg.pin_mck = -1;        // PCM5102不需要MCLK
   cfg.use_apll = false;    // 不使用APLL
   
   i2s.begin(cfg);
   
   // 启用自动重连功能
   a2dp_sink.set_auto_reconnect(true);
   
   // 设置连接状态回调
   a2dp_sink.set_on_connection_state_changed(connection_state_changed);
   a2dp_sink.set_on_audio_state_changed(audio_state_changed);
   
   // 启动A2DP
   a2dp_sink.start("ESP-AI-SPEAKER");
   
   Serial.println("PCM5102音箱已启动，自动重连已启用");
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
 
 
 