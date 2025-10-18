#include <HardwareSerial.h>
#include <mbedtls/sha1.h>
#include <base64.h>
#ifdef ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

// 4G模块串口配置
#define MODEM_TX_PIN 48
#define MODEM_RX_PIN 45
#define MODEM_BAUD 921600

// 服务器配置
#define SERVER_IP "43.139.170.206"
#define SERVER_PORT "5258"

// 串口对象
HardwareSerial modemSerial(1);

// 全局变量
bool isConnected = false;
bool isConfigured = false;
unsigned long lastSendTime = 0;
int sendCounter = 1;
String modemResponse = "";
int stepRetryCount = 0;  // 当前步骤重试计数器
int globalRetryCount = 0; // 全局重试计数器
bool needRestart = false; // 是否需要重启整个流程
unsigned long configStartTime = 0; // 配置开始时间
unsigned long stepStartTime = 0;   // 当前步骤开始时间

// WebSocket相关变量
bool isWebSocketMode = true;  // 启用自定义WebSocket模式
bool wsConnected = false;     // WebSocket连接状态
bool wsHandshakeComplete = false; // WebSocket握手完成状态
String wsBuffer = "";         // WebSocket数据缓冲区
String wsKey = "";            // WebSocket握手密钥

// AT指令配置步骤枚举
enum ConfigStep {
  STEP_AT_TEST = 0,
  STEP_DISABLE_ECHO,
  STEP_GET_ICCID,
  STEP_CHECK_GPRS_ATTACH,
  STEP_SET_TRANSPARENT_MODE,
  STEP_SET_APN,
  STEP_ACTIVATE_GPRS,
  STEP_GET_IP,
  STEP_CONNECT_TCP,
  STEP_COMPLETED
};

ConfigStep currentStep = STEP_AT_TEST;

// 任务信息打印函数
void print_task_info(void) {
#ifdef ESP32
  // 获取当前任务的句柄
  TaskHandle_t currentTaskHandle = xTaskGetCurrentTaskHandle();
  // 获取当前任务的名字
  const char *taskName = pcTaskGetName(currentTaskHandle);
  // 获取当前任务的堆栈高水位标记
  UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(currentTaskHandle);
  Serial.printf("任务名称: %s 剩余堆栈: %d bytes\n", taskName, stackHighWaterMark);
#else
  Serial.println("任务信息打印仅支持ESP32平台");
#endif
}

void setup() {
  // 初始化串口
  Serial.begin(115200);
  modemSerial.begin(MODEM_BAUD, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  
  // 初始化随机数种子
  esp_random();
  
  // 显示启动信息
  printStartupInfo();
  
  delay(2000); // 等待模块启动
  
  // 记录配置开始时间
  configStartTime = millis();
  
  // 开始配置流程
  configure4GModule();
}

void loop() {
  // 处理模块响应
  handleModemResponse();
  
  // 如果配置未完成，检查是否需要重启配置流程
  if (!isConfigured) {
    // 检查配置总超时（5分钟）
    if (millis() - configStartTime > 300000) {
      Serial.println("[TIMEOUT] 配置流程总超时（5分钟），重启配置");
      restartConfigFlow();
    }
    delay(100);
    return;
  }
  
  // 如果已配置完成但连接断开，尝试重连
  if (isConfigured && !isConnected) {
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 10000) { // 每10秒尝试重连一次
      Serial.println("[RECONNECT] 检测到连接断开，尝试重新连接...");
      handleConnectionLoss();
      lastReconnectAttempt = millis();
    }
    delay(100);
    return;
  }
  
  // 如果已配置完成且连接成功，进行数据收发
  if (isConfigured && isConnected) {
    // WebSocket模式处理
    if (isWebSocketMode) {
      handleWebSocketCommunication(); // 自定义WebSocket处理
    }
    
    // 每15秒发送一次数据（减少发送频率提高稳定性）
    if (millis() - lastSendTime >= 15000) {
      sendTestData();
      lastSendTime = millis();
    }
    
    // 接收数据
    receiveData();
    
    // 监控连接状态
    static unsigned long lastStatusCheck = 0;
    if (millis() - lastStatusCheck > 60000) { // 每分钟检查一次详细状态
      printConnectionStatus();
      lastStatusCheck = millis();
    }
  }
  
  // 定期打印任务信息（降低频率以减少串口输出）
  static unsigned long lastTaskInfo = 0;
  if (millis() - lastTaskInfo > 5000) { // 每5秒打印一次
    print_task_info();
    lastTaskInfo = millis();
  }
  
  delay(100);
}

/**
 * 打印连接状态信息
 */
void printConnectionStatus() {
  Serial.println("========== 连接状态报告 ==========");
  Serial.println("配置状态: " + String(isConfigured ? "已完成" : "未完成"));
  Serial.println("TCP连接: " + String(isConnected ? "已连接" : "未连接"));
  Serial.println("WebSocket模式: " + String(isWebSocketMode ? "启用" : "禁用"));
  if (isWebSocketMode) {
    Serial.println("WebSocket连接: " + String(wsConnected ? "已连接" : "未连接"));
    Serial.println("WebSocket握手: " + String(wsHandshakeComplete ? "完成" : "未完成"));
  }
  Serial.println("运行时间: " + String(millis() / 1000) + " 秒");
  Serial.println("发送计数: " + String(sendCounter - 1));
  Serial.println("全局重试次数: " + String(globalRetryCount));
  Serial.println("================================");
}

/**
 * 重启配置流程
 */
void restartConfigFlow() {
  Serial.println("[RESTART] 重置所有配置状态...");
  
  // 重置所有状态变量
  currentStep = STEP_AT_TEST;
  stepRetryCount = 0;
  isConnected = false;
  isConfigured = false;
  wsConnected = false;
  wsHandshakeComplete = false;
  wsBuffer = "";
  wsKey = "";
  
  // 重置时间记录
  configStartTime = millis();
  stepStartTime = 0;
  
  // 清空串口缓冲区
  while (modemSerial.available()) {
    modemSerial.read();
  }
  
  Serial.println("[RESTART] 开始重新配置4G模块...");
  delay(2000);
  
  // 重新开始配置流程
  configure4GModule();
}

/**
 * 配置4G模块
 */
void configure4GModule() {
  executeConfigStep();
}

/**
 * 执行当前配置步骤
 */
void executeConfigStep() {
  String command = "";
  String stepName = "";
  
  switch (currentStep) {
    case STEP_AT_TEST:
      command = "AT";
      stepName = "测试AT通信";
      break;
      
    case STEP_DISABLE_ECHO:
      command = "ATEO";
      stepName = "关闭回显";
      break;
      
    case STEP_GET_ICCID:
      command = "AT+ICCID";
      stepName = "获取SIM卡ICCID";
      break;
      
    case STEP_CHECK_GPRS_ATTACH:
      command = "AT+CGATT?";
      stepName = "检查GPRS附着状态";
      break;
      
    case STEP_SET_TRANSPARENT_MODE:
      command = "AT+CIPMODE=1";
      stepName = "设置透传模式";
      break;
      
    case STEP_SET_APN:
      command = "AT+CSTT=\"\",\"\",\"\"";
      stepName = "设置APN";
      break;
      
    case STEP_ACTIVATE_GPRS:
      command = "AT+CIICR";
      stepName = "激活GPRS";
      break;
      
    case STEP_GET_IP:
      command = "AT+CIFSR";
      stepName = "获取IP地址";
      break;
      
    case STEP_CONNECT_TCP:
      command = "AT+CIPSTART=\"TCP\",\"" + String(SERVER_IP) + "\"," + String(SERVER_PORT);
      stepName = "建立TCP连接";
      break;
      
    case STEP_COMPLETED:
      Serial.println("[SUCCESS] 4G模块配置完成！");
      isConfigured = true;
      return;
  }
  
  Serial.println("[STEP " + String(currentStep + 1) + "] " + stepName);
  Serial.println("[SEND] " + command);
  
  // 记录步骤开始时间
  stepStartTime = millis();
  
  // 发送AT指令
  modemSerial.println(command);
  
  // 等待响应
  unsigned long startTime = millis();
  modemResponse = "";
  
  while (millis() - startTime < 500) { // 等待500ms
    if (modemSerial.available()) {
      modemResponse += modemSerial.readString();
    }
    delay(10);
  }
  
  // 处理响应
  processResponse(stepName);
}

/**
 * 处理AT指令响应
 */
void processResponse(String stepName) {
  modemResponse.trim();
  Serial.println("[RECV] " + modemResponse);
  
  bool success = false;
  
  switch (currentStep) {
    case STEP_AT_TEST:
      success = modemResponse.indexOf("OK") != -1;
      if (!success) {
        Serial.println("[ERROR] 模块无响应或响应异常，可能需要重启");
      }
      break;
      
    case STEP_DISABLE_ECHO:
      // 关闭回显可能返回OK或NO CARRIER，都认为成功
      success = modemResponse.indexOf("OK") != -1 || 
                modemResponse.indexOf("NO CARRIER") != -1;
      if (success) {
        Serial.println("[INFO] 回显已关闭");
      }
      break;
      
    case STEP_GET_ICCID:
      success = modemResponse.indexOf("+ICCID:") != -1 && modemResponse.indexOf("OK") != -1;
      if (success) {
        int start = modemResponse.indexOf("+ICCID: ") + 8;
        int end = modemResponse.indexOf("\n", start);
        String iccid = modemResponse.substring(start, end);
        iccid.trim();
        Serial.println("[INFO] SIM卡ICCID: " + iccid);
      } else {
        Serial.println("[ERROR] 获取SIM卡信息失败，请检查SIM卡是否正确插入");
      }
      break;
      
    case STEP_CHECK_GPRS_ATTACH:
      if (modemResponse.indexOf("+CGATT: 1") != -1 && modemResponse.indexOf("OK") != -1) {
        success = true;
        Serial.println("[INFO] GPRS已附着到网络");
      } else if (modemResponse.indexOf("+CGATT: 0") != -1) {
        success = false;
        Serial.println("[ERROR] GPRS未附着到网络，信号可能不佳或SIM卡问题");
      } else {
        success = false;
        Serial.println("[ERROR] 检查GPRS附着状态失败");
      }
      break;
      
    case STEP_SET_TRANSPARENT_MODE:
      // 设置透传模式：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (modemResponse.indexOf("OK") != -1) {
        Serial.println("[INFO] 透传模式设置成功");
      } else {
        Serial.println("[WARNING] 透传模式设置失败，但继续下一步");
      }
      break;
      
    case STEP_SET_APN:
      // 设置APN：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (modemResponse.indexOf("OK") != -1) {
        Serial.println("[INFO] APN设置成功");
      } else {
        Serial.println("[WARNING] APN设置失败，但继续下一步");
      }
      break;
      
    case STEP_ACTIVATE_GPRS:
      // 激活GPRS：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (modemResponse.indexOf("OK") != -1) {
        Serial.println("[INFO] GPRS激活成功");
      } else {
        Serial.println("[WARNING] GPRS激活失败，但继续下一步");
      }
      break;
      
    case STEP_GET_IP:
      // 查询IP：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (modemResponse.length() > 7 && 
          modemResponse.indexOf(".") != -1 && 
          !modemResponse.startsWith("ERROR")) {
        String ip = modemResponse;
        ip.replace("\r", "");
        ip.replace("\n", "");
        ip.trim();
        Serial.println("[INFO] 获取到IP地址: " + ip);
      } else {
        Serial.println("[WARNING] 获取IP地址失败，但继续下一步");
      }
      break;
      
    case STEP_CONNECT_TCP:
      // TCP连接：检查是否收到OK，如果同时收到CONNECT就直接成功
      if (modemResponse.indexOf("OK") != -1) {
        Serial.println("[INFO] TCP连接命令发送成功");
        
        // 检查是否已经收到CONNECT
        if (modemResponse.indexOf("CONNECT") != -1 && modemResponse.indexOf("CONNECT FAIL") == -1) {
          Serial.println("[SUCCESS] TCP连接已建立！（同时收到OK和CONNECT）");
          isConnected = true;
          
          // 如果启用WebSocket模式，启动握手
          if (isWebSocketMode && !wsHandshakeComplete) {
            startWebSocketHandshake();
          }
          
          success = true;
        } else {
          Serial.println("[INFO] 等待CONNECT确认...");
          success = waitForConnect(); // 等待CONNECT响应
        }
      } else {
        success = false;
        Serial.println("[ERROR] TCP连接命令发送失败");
      }
      break;
  }
  
  if (success) {
    Serial.println("[SUCCESS] " + stepName + " - 成功");
    stepRetryCount = 0; // 重置重试计数器
    
    // 如果是TCP连接步骤，需要等待实际连接完成
    if (currentStep == STEP_CONNECT_TCP) {
      if (isConnected) {
        currentStep = STEP_COMPLETED;
        isConfigured = true;
        globalRetryCount = 0; // 重置全局重试计数器
        Serial.println("[INFO] 4G模块配置完成，开始数据收发测试...");
      } else {
        // 连接失败，重试
        Serial.println("[ERROR] " + stepName + " - 连接失败，准备重试");
        delay(2000);
        Serial.println("[RETRY] 重试当前步骤...");
        executeConfigStep();
      }
    } else {
      // 其他步骤正常处理
      currentStep = (ConfigStep)(currentStep + 1);
      
      if (currentStep < STEP_COMPLETED) {
        delay(1000); // 步骤间延时
        executeConfigStep(); // 执行下一步
      } else {
        isConfigured = true;
        globalRetryCount = 0; // 重置全局重试计数器
        Serial.println("[INFO] 4G模块配置完成，开始数据收发测试...");
      }
    }
  } else {
    Serial.println("[ERROR] " + stepName + " - 失败");
    Serial.println("[ERROR] 响应内容: " + modemResponse);
    
    stepRetryCount++; // 增加重试计数
    
    // 特殊处理：这些步骤不需要重试，直接跳过重试逻辑
    if (currentStep == STEP_SET_TRANSPARENT_MODE || 
        currentStep == STEP_SET_APN || 
        currentStep == STEP_ACTIVATE_GPRS || 
        currentStep == STEP_GET_IP) {
      Serial.println("[INFO] 步骤 " + stepName + " 不进行重试，直接继续下一步");
      // 这些步骤在processResponse中已经强制设置为成功，这里不应该执行到
      return;
    }
    
    // 检查是否达到单步重试上限
    if (stepRetryCount >= 3) {
      Serial.println("[ERROR] 步骤 " + stepName + " 重试" + String(stepRetryCount) + "次仍失败");
      globalRetryCount++;
      
      // 检查全局重试次数
      if (globalRetryCount >= 3) {
        Serial.println("[FATAL] 配置流程失败次数过多，停止配置");
        Serial.println("[INFO] 请检查：");
        Serial.println("1. SIM卡是否正确插入");
        Serial.println("2. 网络信号是否良好");
        Serial.println("3. APN配置是否正确");
        Serial.println("4. 服务器地址和端口是否可达");
        return; // 停止配置流程
      }
      
      // 重启整个配置流程
      Serial.println("[RESTART] 重启整个配置流程... (全局第" + String(globalRetryCount) + "次)");
      delay(5000); // 等待5秒后重启
      restartConfigFlow();
      return;
    }
    
    // 重试当前步骤
    delay(2000);
    Serial.println("[RETRY] 重试当前步骤... (第" + String(stepRetryCount) + "次)");
    executeConfigStep();
  }
}

/**
 * 等待TCP连接确认
 */
bool waitForConnect() {
  Serial.println("[INFO] 等待TCP连接确认...");
  unsigned long startTime = millis();
  String response = "";
  
  // 首先检查是否已经在modemResponse中收到了CONNECT
  if (modemResponse.indexOf("CONNECT") != -1 && modemResponse.indexOf("CONNECT FAIL") == -1) {
    Serial.println("[SUCCESS] TCP连接已建立！（已在响应中检测到）");
    isConnected = true;
    
    // 如果启用WebSocket模式，启动握手
    if (isWebSocketMode && !wsHandshakeComplete) {
      startWebSocketHandshake();
    }
    
    return true;
  }
  
  while (millis() - startTime < 5000) { // 缩短等待时间到5秒
    if (modemSerial.available()) {
      String newData = modemSerial.readString();
      response += newData;
      Serial.println("[RECV] " + newData); // 添加调试输出
      
      // 判断连接成功
      if (response.indexOf("CONNECT") != -1 && response.indexOf("CONNECT FAIL") == -1) {
        Serial.println("[SUCCESS] TCP连接已建立！");
        isConnected = true;
        
        // 如果启用WebSocket模式，启动握手
        if (isWebSocketMode && !wsHandshakeComplete) {
          startWebSocketHandshake();
        }
        
        return true;
      }
      
      // 明确的失败情况
      if (response.indexOf("CONNECT FAIL") != -1 || 
          response.indexOf("ERROR") != -1) {
        Serial.println("[ERROR] 连接失败: " + response);
        isConnected = false;
        return false;
      }
    }
    delay(100);
  }
  
  // 超时后最后一次检查
  if (response.indexOf("CONNECT") != -1 && response.indexOf("CONNECT FAIL") == -1) {
    Serial.println("[SUCCESS] TCP连接已建立！（超时前检测到）");
    isConnected = true;
    
    // 如果启用WebSocket模式，启动握手
    if (isWebSocketMode && !wsHandshakeComplete) {
      startWebSocketHandshake();
    }
    
    return true;
  }
  
  Serial.println("[ERROR] 连接确认超时");
  isConnected = false;
  return false;
}

/**
 * 生成WebSocket密钥
 */
  String generateWebSocketKey() {
    String key = "";
    for (int i = 0; i < 16; i++) {
      key += char(esp_random() % 256);
    }
    return base64::encode(key);
  }

/**
 * 计算WebSocket Accept值
 */
String calculateWebSocketAccept(String key) {
  String combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  
  uint8_t hash[20];
  mbedtls_sha1_context ctx;
  mbedtls_sha1_init(&ctx);
  mbedtls_sha1_starts_ret(&ctx);
  mbedtls_sha1_update_ret(&ctx, (const unsigned char*)combined.c_str(), combined.length());
  mbedtls_sha1_finish_ret(&ctx, hash);
  mbedtls_sha1_free(&ctx);
  
  String hashStr = "";
  for (int i = 0; i < 20; i++) {
    hashStr += char(hash[i]);
  }
  
  return base64::encode(hashStr);
}

/**
 * 启动WebSocket握手
 */
void startWebSocketHandshake() {
  Serial.println("[WS] 启动WebSocket握手...");
  
  wsKey = generateWebSocketKey();
  
  String handshake = "GET / HTTP/1.1\r\n";
  handshake += "Host: " + String(SERVER_IP) + ":" + String(SERVER_PORT) + "\r\n";
  handshake += "Upgrade: websocket\r\n";
  handshake += "Connection: Upgrade\r\n";
  handshake += "Sec-WebSocket-Key: " + wsKey + "\r\n";
  handshake += "Sec-WebSocket-Version: 13\r\n";
  handshake += "\r\n";
  
  Serial.println("[WS] 发送握手请求:");
  Serial.print(handshake);
  modemSerial.print(handshake);
  
  wsHandshakeComplete = false;
}

/**
 * 处理WebSocket通信
 */
void handleWebSocketCommunication() {
  if (!wsHandshakeComplete) {
    // 检查握手响应
    checkWebSocketHandshakeResponse();
  } else {
    // 处理WebSocket数据帧
    processWebSocketFrames();
  }
}

/**
 * 检查WebSocket握手响应
 */
void checkWebSocketHandshakeResponse() {
  if (modemSerial.available()) {
    String response = modemSerial.readString();
    wsBuffer += response;
    
    Serial.println("[WS] 收到握手响应: " + response);
    
    // 检查是否收到完整的HTTP响应
    if (wsBuffer.indexOf("\r\n\r\n") != -1) {
      if (wsBuffer.indexOf("HTTP/1.1 101") != -1 && 
          wsBuffer.indexOf("Upgrade: websocket") != -1) {
        
        // 验证Sec-WebSocket-Accept
        String expectedAccept = calculateWebSocketAccept(wsKey);
        if (wsBuffer.indexOf("Sec-WebSocket-Accept: " + expectedAccept) != -1) {
          Serial.println("[WS] 握手成功！");
          wsHandshakeComplete = true;
          wsConnected = true;
          wsBuffer = ""; // 清空缓冲区
          
          // 发送连接确认消息
          sendWebSocketTextFrame("ESP32S3 Connected");
        } else {
          Serial.println("[WS] 握手失败：Accept值不匹配");
        }
      } else {
        Serial.println("[WS] 握手失败：服务器拒绝升级");
      }
    }
  }
}

/**
 * 生成WebSocket掩码
 */
  void generateWebSocketMask(uint8_t* mask) {
    for (int i = 0; i < 4; i++) {
      mask[i] = esp_random() % 256;
    }
  }

/**
 * 发送WebSocket文本帧
 */
void sendWebSocketTextFrame(String payload) {
  uint8_t* frame = new uint8_t[payload.length() + 14]; // 预留足够空间
  int frameIndex = 0;
  
  // 第一个字节：FIN=1, RSV=000, Opcode=0001 (文本帧)
  frame[frameIndex++] = 0x81;
  
  // 第二个字节开始：MASK=1, Payload length
  if (payload.length() < 126) {
    frame[frameIndex++] = 0x80 | payload.length();
  } else if (payload.length() < 65536) {
    frame[frameIndex++] = 0x80 | 126;
    frame[frameIndex++] = (payload.length() >> 8) & 0xFF;
    frame[frameIndex++] = payload.length() & 0xFF;
  } else {
    // 不支持大于64KB的数据
    delete[] frame;
    Serial.println("[WS] 数据太大，不支持发送");
    return;
  }
  
  // 生成4字节掩码
  uint8_t mask[4];
  generateWebSocketMask(mask);
  for (int i = 0; i < 4; i++) {
    frame[frameIndex++] = mask[i];
  }
  
  // 应用掩码到载荷数据
  for (int i = 0; i < payload.length(); i++) {
    frame[frameIndex++] = payload[i] ^ mask[i % 4];
  }
  
  // 发送帧
  modemSerial.write(frame, frameIndex);
  Serial.println("[WS] 发送文本: " + payload);
  
  delete[] frame;
}

/**
 * 处理WebSocket数据帧
 */
void processWebSocketFrames() {
  if (modemSerial.available()) {
    uint8_t* buffer = new uint8_t[1024];
    int bytesRead = modemSerial.readBytes(buffer, 1024);
    
    if (bytesRead >= 2) {
      int pos = 0;
      
      uint8_t firstByte = buffer[pos++];
      uint8_t secondByte = buffer[pos++];
      
      bool fin = (firstByte & 0x80) != 0;
      uint8_t opcode = firstByte & 0x0F;
      bool masked = (secondByte & 0x80) != 0;
      uint8_t payloadLen = secondByte & 0x7F;
      
      // 读取扩展载荷长度
      if (payloadLen == 126) {
        if (pos + 2 <= bytesRead) {
          payloadLen = (buffer[pos] << 8) | buffer[pos + 1];
          pos += 2;
        }
      }
      
      // 读取掩码（如果有）
      uint8_t mask[4] = {0};
      if (masked) {
        if (pos + 4 <= bytesRead) {
          for (int i = 0; i < 4; i++) {
            mask[i] = buffer[pos++];
          }
        }
      }
      
      // 读取载荷数据
      if (pos + payloadLen <= bytesRead) {
        String payload = "";
        for (int i = 0; i < payloadLen; i++) {
          uint8_t byte = buffer[pos + i];
          if (masked) {
            byte ^= mask[i % 4];
          }
          payload += char(byte);
        }
        
        // 处理不同类型的帧
        switch (opcode) {
          case 0x1: // 文本帧
            Serial.println("[WS] 收到文本: " + payload);
            break;
          case 0x2: // 二进制帧
            Serial.println("[WS] 收到二进制数据，长度: " + String(payloadLen));
            break;
          case 0x8: // 关闭帧
            Serial.println("[WS] 收到关闭帧");
            wsConnected = false;
            break;
          case 0x9: // Ping帧
            Serial.println("[WS] 收到Ping，发送Pong");
            sendWebSocketPongFrame(payload);
            break;
          case 0xA: // Pong帧
            Serial.println("[WS] 收到Pong");
            break;
        }
      }
    }
    
    delete[] buffer;
  }
}

/**
 * 发送WebSocket Pong帧
 */
void sendWebSocketPongFrame(String payload) {
  uint8_t* frame = new uint8_t[payload.length() + 10];
  int frameIndex = 0;
  
  // 第一个字节：FIN=1, RSV=000, Opcode=1010 (Pong帧)
  frame[frameIndex++] = 0x8A;
  
  // 第二个字节：MASK=1, Payload length
  frame[frameIndex++] = 0x80 | payload.length();
  
  // 生成4字节掩码
  uint8_t mask[4];
  generateWebSocketMask(mask);
  for (int i = 0; i < 4; i++) {
    frame[frameIndex++] = mask[i];
  }
  
  // 应用掩码到载荷数据
  for (int i = 0; i < payload.length(); i++) {
    frame[frameIndex++] = payload[i] ^ mask[i % 4];
  }
  
  modemSerial.write(frame, frameIndex);
  delete[] frame;
}

/**
 * 发送测试数据
 */
void sendTestData() {
  String data = "Test Data #" + String(sendCounter) + " from ESP32S3";
  
  if (isWebSocketMode && wsConnected) {
    // 使用自定义WebSocket协议发送数据
    sendWebSocketTextFrame(data);
  } else if (isConnected) {
    // 普通TCP模式
    Serial.println("[SEND TCP] " + data);
    modemSerial.println(data);
  }
  
  sendCounter++;
}

/**
 * 接收数据
 */
void receiveData() {
  // WebSocket模式的数据接收在handleWebSocketCommunication中处理
  if (!isWebSocketMode && modemSerial.available()) {
    // 普通TCP模式
    String receivedData = modemSerial.readString();
    receivedData.trim();
    
    if (receivedData.length() > 0) {
      Serial.println("[RECV TCP] " + receivedData);
    }
  }
}

/**
 * 处理模块响应（用于配置完成后的数据模式）
 */
void handleModemResponse() {
  // 在配置阶段，响应处理已在executeConfigStep中完成
  // 这里主要用于数据传输阶段的异常处理
  
  if (!isConfigured) {
    return; // 配置阶段不需要额外处理
  }
  
  // 如果是WebSocket模式，不在这里处理数据，让WebSocket处理函数处理
  if (isWebSocketMode) {
    // 但仍然需要检查连接状态
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 60000) { // 每60秒检查一次
      checkConnectionStatus();
      lastCheck = millis();
    }
    return; // WebSocket模式下直接返回，不处理串口数据
  }
  
  // 非WebSocket模式下的处理
  if (modemSerial.available()) {
    String response = modemSerial.readString();
    
    // 检查连接断开的关键字
    if (response.indexOf("CLOSED") != -1 || 
        response.indexOf("DISCONNECT") != -1 ||
        response.indexOf("NO CARRIER") != -1) {
      Serial.println("[ERROR] 检测到连接断开: " + response);
      handleConnectionLoss();
      return;
    }
    
    // 检查网络异常
    if (response.indexOf("PDP DEACT") != -1) {
      Serial.println("[ERROR] PDP上下文失效，网络连接丢失");
      handleNetworkLoss();
      return;
    }
    
    // 处理普通TCP数据
    if (response.length() > 0) {
      response.trim();
      Serial.println("[RECV TCP] " + response);
    }
  }
}

/**
 * 处理连接丢失
 */
void handleConnectionLoss() {
  Serial.println("[HANDLE] 处理连接丢失...");
  
  isConnected = false;
  wsConnected = false;
  wsHandshakeComplete = false;
  
  // 尝试重新建立TCP连接
  Serial.println("[RECONNECT] 尝试重新建立TCP连接...");
  currentStep = STEP_CONNECT_TCP;
  stepRetryCount = 0;
  
  delay(2000);
  executeConfigStep();
}

/**
 * 处理网络丢失
 */
void handleNetworkLoss() {
  Serial.println("[HANDLE] 处理网络丢失...");
  
  isConnected = false;
  isConfigured = false;
  wsConnected = false;
  wsHandshakeComplete = false;
  
  // 从GPRS激活步骤重新开始
  Serial.println("[RECONNECT] 从GPRS激活步骤重新开始...");
  currentStep = STEP_ACTIVATE_GPRS;
  stepRetryCount = 0;
  
  delay(3000);
  executeConfigStep();
}

/**
 * 检查连接状态
 */
void checkConnectionStatus() {
  if (!isConnected) {
    return;
  }
  
  // 发送心跳数据检查连接
  String heartbeat = "PING_" + String(millis());
  
  if (isWebSocketMode && wsConnected) {
    sendWebSocketTextFrame(heartbeat);
    Serial.println("[HEARTBEAT] WebSocket心跳已发送");
  } else {
    modemSerial.println(heartbeat);
    Serial.println("[HEARTBEAT] TCP心跳已发送");
  }
}

/**
 * 打印启动信息
 */
void printStartupInfo() {
  Serial.println("========================================");
  Serial.println("ESP32S3 4G模块智能控制程序");
  Serial.println("版本: 2.0.0 - 增强版");
  Serial.println("编译时间: " + String(__DATE__) + " " + String(__TIME__));
  Serial.println("========================================");
  Serial.println("硬件配置:");
  Serial.println("- ESP32S3 TX: GPIO" + String(MODEM_TX_PIN));
  Serial.println("- ESP32S3 RX: GPIO" + String(MODEM_RX_PIN));
  Serial.println("- 波特率: " + String(MODEM_BAUD));
  Serial.println("========================================");
  Serial.println("服务器配置:");
  Serial.println("- IP: " + String(SERVER_IP));
  Serial.println("- 端口: " + String(SERVER_PORT));
  Serial.println("========================================");
  Serial.println("功能特性:");
  Serial.println("- WebSocket模式: " + String(isWebSocketMode ? "启用" : "禁用"));
  Serial.println("- 自动重连: 启用");
  Serial.println("- 智能重试: 启用");
  Serial.println("- 连接监控: 启用");
  Serial.println("- 异常恢复: 启用");
  Serial.println("========================================");
  Serial.println("开始配置4G模块...");
}
