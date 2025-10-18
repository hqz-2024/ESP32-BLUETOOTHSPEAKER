/**
 * @file example_usage.ino
 * @brief LTE4G_Library 使用示例
 * @author ESP32 4G Library Example
 * @version 1.0.0
 * @date 2024
 */

#include "LTE4G_Library.h"

// 创建4G客户端对象
LTE4G_Client lte4g(48, 45, 921600); // TX=48, RX=45, 波特率921600

// 服务器配置
const String SERVER_IP = "43.139.170.206";
const String SERVER_PORT = "2547";
const String APN = ""; // APN可选，留空使用默认

// 数据接收回调函数
void onDataReceived(String data) {
  Serial.println("收到数据: " + data);
  
  // 可以在这里处理接收到的数据
  if (data.indexOf("LED_ON") >= 0) {
    // 例如：开启LED
    digitalWrite(2, HIGH);
    Serial.println("LED已开启");
  } else if (data.indexOf("LED_OFF") >= 0) {
    // 例如：关闭LED
    digitalWrite(2, LOW);
    Serial.println("LED已关闭");
  }
}

// 连接状态变化回调函数
void onStatusChanged(LTE4G_Status status) {
  switch (status) {
    case LTE4G_DISCONNECTED:
      Serial.println("状态变化: 已断开连接");
      break;
    case LTE4G_CONNECTING:
      Serial.println("状态变化: 正在连接");
      break;
    case LTE4G_CONNECTED:
      Serial.println("状态变化: 已连接");
      break;
    case LTE4G_ERROR:
      Serial.println("状态变化: 连接错误");
      break;
  }
}

// 自定义日志处理回调函数（可选）
void onLogMessage(String message) {
  Serial.println("[自定义日志] " + message);
  // 可以在这里添加日志保存到SD卡等功能
}

void setup() {
  // 初始化串口
  Serial.begin(115200);
  Serial.println("========================================");
  Serial.println("LTE4G_Library 使用示例");
  Serial.println("========================================");
  
  // 初始化LED引脚（示例用）
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  
  // 设置回调函数
  lte4g.setDataCallback(onDataReceived);
  lte4g.setStatusCallback(onStatusChanged);
  lte4g.setLogCallback(onLogMessage); // 可选
  
  // 启用WebSocket模式（可选，默认启用）
  lte4g.enableWebSocket(true);
  
  // 初始化4G模块
  if (lte4g.begin(SERVER_IP, SERVER_PORT, APN)) {
    Serial.println("4G模块初始化成功");
    
    // 配置4G模块
    Serial.println("开始配置4G模块...");
    if (lte4g.configure()) {
      Serial.println("4G模块配置成功！");
      
      // 获取模块信息
      Serial.println("模块信息: " + lte4g.getModuleInfo());
      Serial.println("SIM卡ICCID: " + lte4g.getICCID());
      Serial.println("IP地址: " + lte4g.getIPAddress());
      Serial.println("信号强度: " + String(lte4g.getSignalStrength()));
      
      Serial.println("可以开始发送和接收数据了！");
    } else {
      Serial.println("4G模块配置失败！");
    }
  } else {
    Serial.println("4G模块初始化失败！");
  }
}

void loop() {
  // 处理4G模块数据（必须在loop中调用）
  lte4g.handleData();
  
  // 每5秒打印详细状态
  static unsigned long lastDetailedStatus = 0;
  if (millis() - lastDetailedStatus >= 5000) {
    Serial.println("========== 详细状态报告 ==========");
    Serial.println("配置状态: " + String(lte4g.isConfigured() ? "已完成" : "未完成"));
    Serial.println("TCP连接: " + String(lte4g.isConnected() ? "已连接" : "未连接"));
    Serial.println("WebSocket连接: " + String(lte4g.isWebSocketConnected() ? "已连接" : "未连接"));
    Serial.println("运行时间: " + String(millis() / 1000) + "秒");
    Serial.println("==================================");
    lastDetailedStatus = millis();
  }
  
  // 检查连接状态
  if (lte4g.isConnected()) {
    // 每15秒发送一次测试数据
    static unsigned long lastSendTime = 0;
    if (millis() - lastSendTime >= 15000) {
      String testData = "Hello from ESP32S3! Time: " + String(millis() / 1000) + "s";
      
      if (lte4g.sendData(testData)) {
        Serial.println("测试数据发送成功: " + testData);
      } else {
        Serial.println("测试数据发送失败");
      }
      
      lastSendTime = millis();
    }
    
    // 每分钟发送一次心跳包
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat >= 60000) {
      lte4g.sendHeartbeat();
      lastHeartbeat = millis();
    }
    
    // 每2分钟打印一次统计信息
    static unsigned long lastStats = 0;
    if (millis() - lastStats >= 120000) {
      Serial.println(lte4g.getStatistics());
      lastStats = millis();
    }
    
  } else {
    // 连接断开时的处理
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt >= 30000) { // 每30秒尝试重连一次
      Serial.println("检测到连接断开，尝试重新连接...");
      if (lte4g.reconnect()) {
        Serial.println("重连成功！");
      } else {
        Serial.println("重连失败，将在30秒后再次尝试");
      }
      lastReconnectAttempt = millis();
    }
  }
  
  // 处理串口输入命令（调试用）
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "status") {
      // 显示状态信息
      Serial.println("=== 状态信息 ===");
      Serial.println("连接状态: " + String(lte4g.isConnected() ? "已连接" : "未连接"));
      Serial.println("配置状态: " + String(lte4g.isConfigured() ? "已完成" : "未完成"));
      Serial.println("IP地址: " + lte4g.getIPAddress());
      Serial.println("信号强度: " + String(lte4g.getSignalStrength()));
      Serial.println(lte4g.getStatistics());
    } else if (command == "reset") {
      // 重置模块
      Serial.println("重置4G模块...");
      lte4g.reset();
    } else if (command == "disconnect") {
      // 断开连接
      Serial.println("断开连接...");
      lte4g.disconnect();
    } else if (command.startsWith("send ")) {
      // 发送自定义数据
      String data = command.substring(5);
      if (lte4g.sendData(data)) {
        Serial.println("数据发送成功: " + data);
      } else {
        Serial.println("数据发送失败");
      }
    } else if (command == "help") {
      // 显示帮助信息
      Serial.println("=== 可用命令 ===");
      Serial.println("status - 显示状态信息");
      Serial.println("reset - 重置4G模块");
      Serial.println("disconnect - 断开连接");
      Serial.println("send <数据> - 发送自定义数据");
      Serial.println("help - 显示此帮助信息");
    } else {
      Serial.println("未知命令: " + command + "，输入 'help' 查看帮助");
    }
  }
  
  delay(100); // 避免过于频繁的循环
}

/*
使用说明：

1. 基本使用步骤：
   - 创建LTE4G_Client对象
   - 设置回调函数（可选）
   - 调用begin()初始化
   - 调用configure()配置模块
   - 在loop()中调用handleData()处理数据

2. 主要API函数：
   - begin(serverIP, serverPort, apn) - 初始化模块
   - configure() - 配置模块连接
   - sendData(data) - 发送数据
   - handleData() - 处理接收数据（必须在loop中调用）
   - isConnected() - 检查连接状态
   - reconnect() - 重新连接

3. 回调函数：
   - setDataCallback() - 数据接收回调
   - setStatusCallback() - 状态变化回调
   - setLogCallback() - 日志输出回调

4. 工具函数：
   - getModuleInfo() - 获取模块信息
   - getSignalStrength() - 获取信号强度
   - getIPAddress() - 获取IP地址
   - getStatistics() - 获取统计信息

5. 串口调试命令：
   - "status" - 显示状态
   - "reset" - 重置模块
   - "send <数据>" - 发送数据
   - "help" - 显示帮助
*/ 