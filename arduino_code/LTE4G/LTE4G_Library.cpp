/**
 * @file LTE4G_Library.cpp
 * @brief 4G LTE模块控制库实现
 * @author ESP32 4G Library
 * @version 1.0.0
 * @date 2024
 * @copyright MIT License
 */

#include "LTE4G_Library.h"

// 构造函数
LTE4G_Client::LTE4G_Client(int txPin, int rxPin, int baudRate) {
  _txPin = txPin;
  _rxPin = rxPin;
  _baudRate = baudRate;
  
  // 初始化状态变量
  _status = LTE4G_DISCONNECTED;
  _currentStep = STEP_AT_TEST;
  _isConfigured = false;
  _isConnected = false;
  _isWebSocketMode = true;
  _wsConnected = false;
  _wsHandshakeComplete = false;
  
  // 初始化计数器
  _stepRetryCount = 0;
  _globalRetryCount = 0;
  _sendCounter = 1;
  
  // 初始化时间变量
  _configStartTime = 0;
  _stepStartTime = 0;
  _lastSendTime = 0;
  _lastStatusCheck = 0;
  
  // 初始化回调函数
  _dataCallback = nullptr;
  _statusCallback = nullptr;
  _logCallback = nullptr;
  
  // 创建串口对象
  _modemSerial = new HardwareSerial(1);
}

// 析构函数
LTE4G_Client::~LTE4G_Client() {
  if (_modemSerial) {
    delete _modemSerial;
  }
}

// 初始化4G模块
bool LTE4G_Client::begin(String serverIP, String serverPort, String apn) {
  _serverIP = serverIP;
  _serverPort = serverPort;
  _apn = apn;
  
  // 初始化串口
  _modemSerial->begin(_baudRate, SERIAL_8N1, _rxPin, _txPin);
  
  logInfo("LTE4G_Client 初始化完成");
  logInfo("硬件配置: TX=" + String(_txPin) + ", RX=" + String(_rxPin) + ", 波特率=" + String(_baudRate));
  logInfo("服务器配置: " + _serverIP + ":" + _serverPort);
  
  delay(2000); // 等待模块启动
  
  return true;
}

// 启用WebSocket模式
void LTE4G_Client::enableWebSocket(bool enable) {
  _isWebSocketMode = enable;
  logInfo("WebSocket模式: " + String(enable ? "启用" : "禁用"));
}

// 配置4G模块
bool LTE4G_Client::configure() {
  logInfo("开始配置4G模块...");
  _configStartTime = millis();
  _currentStep = STEP_AT_TEST;
  _stepRetryCount = 0;
  _globalRetryCount = 0;
  
  executeConfigStep();
  
  // 等待配置完成（最多5分钟）
  unsigned long startTime = millis();
  while (!_isConfigured && (millis() - startTime < 300000)) {
    handleData();
    delay(100);
  }
  
  if (_isConfigured) {
    logInfo("4G模块配置成功！");
    _status = LTE4G_CONNECTED;
    if (_statusCallback) {
      _statusCallback(_status);
    }
    return true;
  } else {
    logError("4G模块配置失败！");
    _status = LTE4G_ERROR;
    if (_statusCallback) {
      _statusCallback(_status);
    }
    return false;
  }
}

// 获取连接状态
LTE4G_Status LTE4G_Client::getStatus() {
  return _status;
}

// 发送数据
bool LTE4G_Client::sendData(String data) {
  if (!_isConnected) {
    logError("发送失败：未连接");
    return false;
  }
  
  if (_isWebSocketMode && _wsConnected) {
    sendWebSocketTextFrame(data);
    logInfo("WebSocket发送: " + data);
  } else {
    _modemSerial->println(data);
    logInfo("TCP发送: " + data);
  }
  
  _lastSendTime = millis();
  return true;
}

// 发送二进制数据
bool LTE4G_Client::sendBinaryData(uint8_t* data, size_t length) {
  if (!_isConnected) {
    logError("发送失败：未连接");
    return false;
  }
  
  _modemSerial->write(data, length);
  logInfo("二进制数据发送，长度: " + String(length));
  
  _lastSendTime = millis();
  return true;
}

// 处理接收数据
void LTE4G_Client::handleData() {
  if (!_isConfigured) {
    // 配置阶段的数据处理
    if (_modemSerial->available()) {
      String response = _modemSerial->readString();
      _modemResponse += response;
      
      // 检查是否收到完整响应
      if (_modemResponse.indexOf("OK") != -1 || 
          _modemResponse.indexOf("ERROR") != -1 ||
          _modemResponse.indexOf("CONNECT") != -1 ||
          _modemResponse.indexOf("FAIL") != -1) {
        // 处理配置响应
        processConfigResponse();
      }
    }
    
    // 检查配置超时
    if (millis() - _configStartTime > 300000) {
      logError("配置超时，重启配置流程");
      restartConfigFlow();
    }
    
    return;
  }
  
  // 数据传输阶段的处理
  if (_isWebSocketMode) {
    // WebSocket模式下的数据处理
    if (!_wsHandshakeComplete) {
      // 握手阶段：使用文本模式处理HTTP响应
      checkWebSocketHandshakeResponse();
      
      // 检查握手超时（30秒）
      static unsigned long handshakeStartTime = 0;
      if (handshakeStartTime == 0) {
        handshakeStartTime = millis();
      }
      
      if (millis() - handshakeStartTime > 30000) {
        logError("[WS] 握手超时，重新启动握手");
        handshakeStartTime = millis();
        _wsBuffer = "";
        startWebSocketHandshake();
      }
    } else {
      // 数据传输阶段：使用二进制模式处理WebSocket帧
      processWebSocketFrames();
      
      // WebSocket心跳机制
      static unsigned long lastPing = 0;
      if (millis() - lastPing > 30000) { // 每30秒发送一次ping
        if (_wsConnected) {
          sendWebSocketPing();
          lastPing = millis();
        }
      }
    }
  } else {
    // 普通TCP模式
    if (_modemSerial->available()) {
      String receivedData = _modemSerial->readString();
      receivedData.trim();
      
      if (receivedData.length() > 0) {
        logInfo("TCP接收: " + receivedData);
        Serial.println("[TCP] 收到消息: " + receivedData); // 直接显示
        if (_dataCallback) {
          _dataCallback(receivedData);
        }
      }
    }
  }
  
  // 检查连接状态和错误处理
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 60000) {
    checkConnectionHealth();
    lastCheck = millis();
  }
}

// 设置数据回调函数
void LTE4G_Client::setDataCallback(LTE4G_DataCallback callback) {
  _dataCallback = callback;
}

// 设置状态回调函数
void LTE4G_Client::setStatusCallback(LTE4G_StatusCallback callback) {
  _statusCallback = callback;
}

// 设置日志回调函数
void LTE4G_Client::setLogCallback(LTE4G_LogCallback callback) {
  _logCallback = callback;
}

// 获取模块信息
String LTE4G_Client::getModuleInfo() {
  _modemSerial->println("ATI");
  delay(500);
  
  String response = "";
  while (_modemSerial->available()) {
    response += _modemSerial->readString();
  }
  
  return response;
}

// 获取信号强度
int LTE4G_Client::getSignalStrength() {
  _modemSerial->println("AT+CSQ");
  delay(500);
  
  String response = "";
  while (_modemSerial->available()) {
    response += _modemSerial->readString();
  }
  
  int rssi = -1;
  int start = response.indexOf("+CSQ: ");
  if (start != -1) {
    start += 6;
    int end = response.indexOf(",", start);
    if (end != -1) {
      rssi = response.substring(start, end).toInt();
    }
  }
  
  return rssi;
}

// 获取IP地址
String LTE4G_Client::getIPAddress() {
  _modemSerial->println("AT+CIFSR");
  delay(500);
  
  String response = "";
  while (_modemSerial->available()) {
    response += _modemSerial->readString();
  }
  
  response.trim();
  return response;
}

// 获取SIM卡ICCID
String LTE4G_Client::getICCID() {
  _modemSerial->println("AT+ICCID");
  delay(500);
  
  String response = "";
  while (_modemSerial->available()) {
    response += _modemSerial->readString();
  }
  
  int start = response.indexOf("+ICCID: ");
  if (start != -1) {
    start += 8;
    int end = response.indexOf("\n", start);
    if (end != -1) {
      String iccid = response.substring(start, end);
      iccid.trim();
      return iccid;
    }
  }
  
  return "";
}

// 断开连接
void LTE4G_Client::disconnect() {
  logInfo("断开连接...");
  _modemSerial->println("+++");
  delay(1000);
  _modemSerial->println("AT+CIPCLOSE");
  
  _isConnected = false;
  _wsConnected = false;
  _wsHandshakeComplete = false;
  _status = LTE4G_DISCONNECTED;
  
  if (_statusCallback) {
    _statusCallback(_status);
  }
}

// 重新连接
bool LTE4G_Client::reconnect() {
  logInfo("尝试重新连接...");
  
  disconnect();
  delay(2000);
  
  _currentStep = STEP_CONNECT_TCP;
  _stepRetryCount = 0;
  executeConfigStep();
  
  // 等待连接完成
  unsigned long startTime = millis();
  while (!_isConnected && (millis() - startTime < 30000)) {
    handleData();
    delay(100);
  }
  
  return _isConnected;
}

// 发送心跳包
void LTE4G_Client::sendHeartbeat() {
  String heartbeat = "PING_" + String(millis());
  sendData(heartbeat);
  logInfo("心跳包已发送");
}

// 检查是否已连接
bool LTE4G_Client::isConnected() {
  return _isConnected;
}

// 检查是否已配置
bool LTE4G_Client::isConfigured() {
  return _isConfigured;
}

// 检查WebSocket是否已连接
bool LTE4G_Client::isWebSocketConnected() {
  return _wsConnected && _wsHandshakeComplete;
}

// 获取统计信息
String LTE4G_Client::getStatistics() {
  String stats = "========== 统计信息 ==========\n";
  stats += "配置状态: " + String(_isConfigured ? "已完成" : "未完成") + "\n";
  stats += "连接状态: " + String(_isConnected ? "已连接" : "未连接") + "\n";
  stats += "WebSocket模式: " + String(_isWebSocketMode ? "启用" : "禁用") + "\n";
  if (_isWebSocketMode) {
    stats += "WebSocket连接: " + String(_wsConnected ? "已连接" : "未连接") + "\n";
  }
  stats += "运行时间: " + String(millis() / 1000) + " 秒\n";
  stats += "发送计数: " + String(_sendCounter - 1) + "\n";
  stats += "重试次数: " + String(_globalRetryCount) + "\n";
  stats += "=============================";
  
  return stats;
}

// 重置模块
void LTE4G_Client::reset() {
  logInfo("重置4G模块...");
  restartConfigFlow();
}

// 设置超时时间
void LTE4G_Client::setTimeout(unsigned long timeout) {
  // 预留接口，后续可扩展
}

// 启用调试模式
void LTE4G_Client::enableDebug(bool enable) {
  // 预留接口，后续可扩展
}

// =============== 私有方法实现 ===============

// 执行配置步骤
void LTE4G_Client::executeConfigStep() {
  String command = "";
  String stepName = "";
  
  switch (_currentStep) {
    case STEP_AT_TEST:
      command = "AT";
      stepName = "测试AT通信";
      break;
    case STEP_DISABLE_ECHO:
      command = "ATE0";
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
      command = "AT+CSTT=\"\",\"\",\"\"";  // 修复：使用空APN，与原程序一致
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
      command = "AT+CIPSTART=\"TCP\",\"" + _serverIP + "\"," + _serverPort;
      stepName = "建立TCP连接";
      break;
    case STEP_COMPLETED:
      logInfo("4G模块配置完成！");
      _isConfigured = true;
      return;
  }
  
  logInfo("[步骤 " + String(_currentStep + 1) + "] " + stepName);
  logInfo("[发送] " + command);
  
  _stepStartTime = millis();
  _modemSerial->println(command);
  _modemResponse = "";
  
  // 等待响应 - 延长等待时间
  unsigned long startTime = millis();
  while (millis() - startTime < 2000) {  // 增加到2秒
    if (_modemSerial->available()) {
      _modemResponse += _modemSerial->readString();
    }
    delay(10);
  }
  
  processResponse(stepName);
}

// 处理配置响应
void LTE4G_Client::processConfigResponse() {
  // 这个方法在handleData中被调用，用于处理异步响应
  if (_modemResponse.length() > 0) {
    String stepName = getStepName(_currentStep);
    processResponse(stepName);
  }
}

// 获取步骤名称
String LTE4G_Client::getStepName(LTE4G_ConfigStep step) {
  switch (step) {
    case STEP_AT_TEST: return "测试AT通信";
    case STEP_DISABLE_ECHO: return "关闭回显";
    case STEP_GET_ICCID: return "获取SIM卡ICCID";
    case STEP_CHECK_GPRS_ATTACH: return "检查GPRS附着状态";
    case STEP_SET_TRANSPARENT_MODE: return "设置透传模式";
    case STEP_SET_APN: return "设置APN";
    case STEP_ACTIVATE_GPRS: return "激活GPRS";
    case STEP_GET_IP: return "获取IP地址";
    case STEP_CONNECT_TCP: return "建立TCP连接";
    default: return "未知步骤";
  }
}

// 处理响应
void LTE4G_Client::processResponse(String stepName) {
  _modemResponse.trim();
  logInfo("[接收] " + _modemResponse);
  
  bool success = false;
  
  switch (_currentStep) {
    case STEP_AT_TEST:
      success = _modemResponse.indexOf("OK") != -1;
      if (!success) {
        logError("模块无响应或响应异常，可能需要重启");
      }
      break;
      
    case STEP_DISABLE_ECHO:
      // 关闭回显可能返回OK或NO CARRIER，都认为成功
      success = _modemResponse.indexOf("OK") != -1 || 
                _modemResponse.indexOf("NO CARRIER") != -1;
      if (success) {
        logInfo("回显已关闭");
      }
      break;
      
    case STEP_GET_ICCID:
      success = _modemResponse.indexOf("+ICCID:") != -1 && _modemResponse.indexOf("OK") != -1;
      if (success) {
        int start = _modemResponse.indexOf("+ICCID: ") + 8;
        int end = _modemResponse.indexOf("\n", start);
        String iccid = _modemResponse.substring(start, end);
        iccid.trim();
        logInfo("SIM卡ICCID: " + iccid);
      } else {
        logError("获取SIM卡信息失败，请检查SIM卡是否正确插入");
      }
      break;
      
    case STEP_CHECK_GPRS_ATTACH:
      if (_modemResponse.indexOf("+CGATT: 1") != -1 && _modemResponse.indexOf("OK") != -1) {
        success = true;
        logInfo("GPRS已附着到网络");
      } else if (_modemResponse.indexOf("+CGATT: 0") != -1) {
        success = false;
        logError("GPRS未附着到网络，信号可能不佳或SIM卡问题");
      } else {
        success = false;
        logError("检查GPRS附着状态失败");
      }
      break;
      
    case STEP_SET_TRANSPARENT_MODE:
      // 设置透传模式：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (_modemResponse.indexOf("OK") != -1) {
        logInfo("透传模式设置成功");
      } else {
        logInfo("透传模式设置失败，但继续下一步");
      }
      break;
      
    case STEP_SET_APN:
      // 设置APN：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (_modemResponse.indexOf("OK") != -1) {
        logInfo("APN设置成功");
      } else {
        logInfo("APN设置失败，但继续下一步");
      }
      break;
      
    case STEP_ACTIVATE_GPRS:
      // 激活GPRS：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (_modemResponse.indexOf("OK") != -1) {
        logInfo("GPRS激活成功");
      } else {
        logInfo("GPRS激活失败，但继续下一步");
      }
      break;
      
    case STEP_GET_IP:
      // 查询IP：无论成功失败都继续下一步
      success = true; // 强制成功，直接跳到下一步
      if (_modemResponse.length() > 7 && 
          _modemResponse.indexOf(".") != -1 && 
          !_modemResponse.startsWith("ERROR")) {
        String ip = _modemResponse;
        ip.replace("\r", "");
        ip.replace("\n", "");
        ip.trim();
        logInfo("获取到IP地址: " + ip);
      } else {
        logInfo("获取IP地址失败，但继续下一步");
      }
      break;
      
    case STEP_CONNECT_TCP:
      // TCP连接：检查是否收到OK，如果同时收到CONNECT就直接成功
      if (_modemResponse.indexOf("OK") != -1) {
        logInfo("TCP连接命令发送成功");
        
        // 检查是否已经收到CONNECT
        if (_modemResponse.indexOf("CONNECT") != -1 && _modemResponse.indexOf("CONNECT FAIL") == -1) {
          logInfo("TCP连接已建立！（同时收到OK和CONNECT）");
          _isConnected = true;
          
          // 如果启用WebSocket模式，启动握手
          if (_isWebSocketMode && !_wsHandshakeComplete) {
            startWebSocketHandshake();
          }
          
          success = true;
        } else {
          logInfo("等待CONNECT确认...");
          success = waitForConnect(); // 等待CONNECT响应
        }
      } else {
        success = false;
        logError("TCP连接命令发送失败");
      }
      break;
  }
  
  if (success) {
    logInfo("[成功] " + stepName + " - 成功");
    _stepRetryCount = 0; // 重置重试计数器
    
    // 如果是TCP连接步骤，需要等待实际连接完成
    if (_currentStep == STEP_CONNECT_TCP) {
      if (_isConnected) {
        _currentStep = STEP_COMPLETED;
        _isConfigured = true;
        _globalRetryCount = 0; // 重置全局重试计数器
        logInfo("4G模块配置完成，开始数据收发测试...");
      } else {
        // 连接失败，重试
        logError(stepName + " - 连接失败，准备重试");
        delay(2000);
        logInfo("重试当前步骤...");
        executeConfigStep();
      }
    } else {
      // 其他步骤正常处理
      _currentStep = (LTE4G_ConfigStep)(_currentStep + 1);
      
      if (_currentStep < STEP_COMPLETED) {
        delay(1000); // 步骤间延时
        executeConfigStep(); // 执行下一步
      } else {
        _isConfigured = true;
        _globalRetryCount = 0; // 重置全局重试计数器
        logInfo("4G模块配置完成，开始数据收发测试...");
      }
    }
  } else {
    logError("[失败] " + stepName + " - 失败");
    logError("响应内容: " + _modemResponse);
    
    _stepRetryCount++; // 增加重试计数
    
    // 特殊处理：这些步骤不需要重试，直接跳过重试逻辑
    if (_currentStep == STEP_SET_TRANSPARENT_MODE || 
        _currentStep == STEP_SET_APN || 
        _currentStep == STEP_ACTIVATE_GPRS || 
        _currentStep == STEP_GET_IP) {
      logInfo("步骤 " + stepName + " 不进行重试，直接继续下一步");
      // 这些步骤在processResponse中已经强制设置为成功，这里不应该执行到
      return;
    }
    
    // 检查是否达到单步重试上限
    if (_stepRetryCount >= 3) {
      logError("步骤 " + stepName + " 重试" + String(_stepRetryCount) + "次仍失败");
      _globalRetryCount++;
      
      // 检查全局重试次数
      if (_globalRetryCount >= 3) {
        logError("配置流程失败次数过多，停止配置");
        logInfo("请检查：");
        logInfo("1. SIM卡是否正确插入");
        logInfo("2. 网络信号是否良好");
        logInfo("3. APN配置是否正确");
        logInfo("4. 服务器地址和端口是否可达");
        _status = LTE4G_ERROR;
        return; // 停止配置流程
      }
      
      // 重启整个配置流程
      logInfo("重启整个配置流程... (全局第" + String(_globalRetryCount) + "次)");
      delay(5000); // 等待5秒后重启
      restartConfigFlow();
      return;
    }
    
    // 重试当前步骤
    delay(2000);
    logInfo("重试当前步骤... (第" + String(_stepRetryCount) + "次)");
    executeConfigStep();
  }
}

// 等待TCP连接确认
bool LTE4G_Client::waitForConnect() {
  logInfo("等待TCP连接确认...");
  unsigned long startTime = millis();
  String response = "";
  
  // 首先检查是否已经在modemResponse中收到了CONNECT
  if (_modemResponse.indexOf("CONNECT") != -1 && _modemResponse.indexOf("CONNECT FAIL") == -1) {
    logInfo("TCP连接已建立！（已在响应中检测到）");
    _isConnected = true;
    
    // 清空响应缓冲区，防止干扰WebSocket握手
    _modemResponse = "";
    
    // 如果启用WebSocket模式，立即启动握手
    if (_isWebSocketMode && !_wsHandshakeComplete) {
      delay(1000); // 等待连接稳定
      // 清空串口缓冲区
      while (_modemSerial->available()) {
        _modemSerial->read();
      }
      startWebSocketHandshake();
    }
    
    return true;
  }
  
  while (millis() - startTime < 10000) { // 等待10秒
    if (_modemSerial->available()) {
      String newData = _modemSerial->readString();
      response += newData;
      logInfo("TCP响应: " + newData);
      
      // 判断连接成功
      if (response.indexOf("CONNECT") != -1 && response.indexOf("CONNECT FAIL") == -1) {
        logInfo("TCP连接已建立！");
        _isConnected = true;
        
        // 清空响应缓冲区，防止干扰WebSocket握手
        _modemResponse = "";
        
        // 如果启用WebSocket模式，立即启动握手
        if (_isWebSocketMode && !_wsHandshakeComplete) {
          delay(1000); // 等待连接稳定
          // 清空串口缓冲区
          while (_modemSerial->available()) {
            _modemSerial->read();
          }
          startWebSocketHandshake();
        }
        
        return true;
      }
      
      // 明确的失败情况
      if (response.indexOf("CONNECT FAIL") != -1 || 
          response.indexOf("ERROR") != -1) {
        logError("连接失败: " + response);
        _isConnected = false;
        return false;
      }
    }
    delay(100);
  }
  
  // 超时后最后一次检查
  if (response.indexOf("CONNECT") != -1 && response.indexOf("CONNECT FAIL") == -1) {
    logInfo("TCP连接已建立！（超时前检测到）");
    _isConnected = true;
    
    // 清空响应缓冲区，防止干扰WebSocket握手
    _modemResponse = "";
    
    // 如果启用WebSocket模式，立即启动握手
    if (_isWebSocketMode && !_wsHandshakeComplete) {
      delay(1000); // 等待连接稳定
      // 清空串口缓冲区
      while (_modemSerial->available()) {
        _modemSerial->read();
      }
      startWebSocketHandshake();
    }
    
    return true;
  }
  
  logError("连接确认超时");
  _isConnected = false;
  return false;
}

// 重启配置流程
void LTE4G_Client::restartConfigFlow() {
  logInfo("重启配置流程...");
  
  _currentStep = STEP_AT_TEST;
  _stepRetryCount = 0;
  _isConnected = false;
  _isConfigured = false;
  _wsConnected = false;
  _wsHandshakeComplete = false;
  _configStartTime = millis();
  
  // 清空串口缓冲区
  while (_modemSerial->available()) {
    _modemSerial->read();
  }
  
  delay(2000);
  executeConfigStep();
}

// 处理连接丢失
void LTE4G_Client::handleConnectionLoss() {
  logInfo("处理连接丢失...");
  
  _isConnected = false;
  _wsConnected = false;
  _wsHandshakeComplete = false;
  _status = LTE4G_DISCONNECTED;
  
  if (_statusCallback) {
    _statusCallback(_status);
  }
}

// 处理网络丢失
void LTE4G_Client::handleNetworkLoss() {
  logInfo("处理网络丢失...");
  
  _isConnected = false;
  _isConfigured = false;
  _wsConnected = false;
  _wsHandshakeComplete = false;
  _status = LTE4G_DISCONNECTED;
  
  if (_statusCallback) {
    _statusCallback(_status);
  }
}

// 检查连接健康状态
void LTE4G_Client::checkConnectionHealth() {
  if (!_isConnected) return;
  
  // 发送心跳检查连接
  String heartbeat = "PING_" + String(millis());
  if (_isWebSocketMode && _wsConnected) {
    sendWebSocketTextFrame(heartbeat);
  } else {
    _modemSerial->println(heartbeat);
  }
}

// =============== WebSocket相关方法 ===============

// 生成WebSocket密钥
String LTE4G_Client::generateWebSocketKey() {
  String key = "";
  for (int i = 0; i < 16; i++) {
    key += char(esp_random() % 256);
  }
  return base64::encode(key);
}

// 计算WebSocket Accept值
String LTE4G_Client::calculateWebSocketAccept(String key) {
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

// 启动WebSocket握手
void LTE4G_Client::startWebSocketHandshake() {
  logInfo("[WS] 启动WebSocket握手...");
  
  _wsKey = generateWebSocketKey();
  
  String handshake = "GET / HTTP/1.1\r\n";
  handshake += "Host: " + _serverIP + ":" + _serverPort + "\r\n";
  handshake += "Upgrade: websocket\r\n";
  handshake += "Connection: Upgrade\r\n";
  handshake += "Sec-WebSocket-Key: " + _wsKey + "\r\n";
  handshake += "Sec-WebSocket-Version: 13\r\n";
  handshake += "\r\n";
  
  logInfo("[WS] 发送握手请求:");
  logInfo(handshake);
  _modemSerial->print(handshake);
  
  _wsHandshakeComplete = false;
  _wsBuffer = "";
}

// 检查WebSocket握手响应 - 改进版本
void LTE4G_Client::checkWebSocketHandshakeResponse() {
  if (_modemSerial->available()) {
    String response = _modemSerial->readString();
    _wsBuffer += response;
    
    logInfo("[WS] 收到数据: " + response);
    
    // 检查HTTP响应（优先检查正常握手）
    if (_wsBuffer.indexOf("HTTP/1.1 101") != -1) {
      logInfo("[WS] 握手成功！");
      _wsHandshakeComplete = true;
      _wsConnected = true;
      
      // 检查握手后是否有剩余数据
      int headerEnd = _wsBuffer.indexOf("\r\n\r\n");
      if (headerEnd != -1) {
        headerEnd += 4;
        if (headerEnd < _wsBuffer.length()) {
          // 保留握手后的数据
          String remainingData = _wsBuffer.substring(headerEnd);
          logInfo("[WS] 握手后有数据，长度: " + String(remainingData.length()));
          _wsBuffer = remainingData; // 保留剩余数据
          // 立即处理剩余数据
          if (_wsBuffer.length() > 0) {
            processWebSocketBuffer();
          }
          return;
        }
      }
      
      _wsBuffer = "";
      delay(100); // 稍等一下让握手完全完成
      sendWebSocketTextFrame("ESP32S3 Connected");
      return;
    }
    
    // 检查是否收到了WebSocket数据帧（说明握手已经成功但我们错过了HTTP响应）
    if (_wsBuffer.length() > 0) {
      // 检查第一个字节是否是WebSocket帧标识
      uint8_t firstByte = (uint8_t)_wsBuffer[0];
      if (firstByte == 0x81 || firstByte == 0x82 || firstByte == 0x89 || firstByte == 0x8A) {
        logInfo("[WS] 检测到WebSocket数据帧，握手已成功");
        _wsHandshakeComplete = true;
        _wsConnected = true;
        // 保留缓冲区数据供后续处理
        return;
      }
    }
    
    // 检查是否握手失败
    if (_wsBuffer.indexOf("CLOSED") != -1 || 
        _wsBuffer.indexOf("ERROR") != -1 ||
        _wsBuffer.indexOf("400") != -1 ||
        _wsBuffer.indexOf("404") != -1) {
      logError("[WS] 握手失败: " + _wsBuffer);
      _wsConnected = false;
      _wsHandshakeComplete = false;
      return;
    }
    
    // 限制缓冲区大小
    if (_wsBuffer.length() > 1024) {
      logError("[WS] 握手响应过长，重置");
      _wsBuffer = "";
    }
  }
}

// 处理WebSocket数据帧 - 改进版本
void LTE4G_Client::processWebSocketFrames() {
  // 处理缓冲区中的数据
  if (_wsBuffer.length() > 0) {
    logInfo("[WS] 处理缓冲区数据，长度: " + String(_wsBuffer.length()));
    processWebSocketBuffer();
  }
  
  // 处理新数据
  if (_modemSerial->available()) {
    String newData = _modemSerial->readString();
    processWebSocketData(newData);
  }
}

// 处理WebSocket缓冲区数据 - 改进版本
void LTE4G_Client::processWebSocketBuffer() {
  if (_wsBuffer.length() < 2) return;
  
  uint8_t firstByte = (uint8_t)_wsBuffer[0];
  uint8_t secondByte = (uint8_t)_wsBuffer[1];
  
  // 检查是否是有效的WebSocket帧
  if ((firstByte & 0x80) == 0) {
    logError("[WS] 无效帧，FIN位未设置");
    _wsBuffer = "";
    return;
  }
  
  uint8_t opcode = firstByte & 0x0F;
  bool masked = (secondByte & 0x80) != 0;
  uint8_t payloadLen = secondByte & 0x7F;
  
  logInfo("[WS] 帧解析 - Opcode: " + String(opcode, HEX) + " Masked: " + String(masked) + " Len: " + String(payloadLen));
  
  int headerSize = 2; // 基本头部大小
  
  // 处理扩展长度
  if (payloadLen == 126) {
    if (_wsBuffer.length() < 4) {
      logInfo("[WS] 等待扩展长度数据");
      return;
    }
    payloadLen = ((uint8_t)_wsBuffer[2] << 8) | (uint8_t)_wsBuffer[3];
    headerSize = 4;
    logInfo("[WS] 扩展长度: " + String(payloadLen));
  } else if (payloadLen == 127) {
    logError("[WS] 不支持64位长度");
    _wsBuffer = "";
    return;
  }
  
  // 处理掩码
  if (masked) {
    headerSize += 4;
  }
  
  // 检查是否有完整的帧
  if (_wsBuffer.length() < headerSize + payloadLen) {
    logInfo("[WS] 帧不完整，等待更多数据. 需要: " + String(headerSize + payloadLen) + " 有: " + String(_wsBuffer.length()));
    return;
  }
  
  // 提取载荷数据
  String payload = "";
  int payloadStart = headerSize;
  
  if (masked) {
    // 获取掩码
    uint8_t mask[4];
    for (int i = 0; i < 4; i++) {
      mask[i] = (uint8_t)_wsBuffer[2 + (headerSize == 8 ? 2 : 0) + i];
    }
    
    // 解码载荷
    for (int i = 0; i < payloadLen; i++) {
      uint8_t byte = (uint8_t)_wsBuffer[payloadStart + i];
      byte ^= mask[i % 4];
      payload += char(byte);
    }
  } else {
    // 无掩码，直接提取
    for (int i = 0; i < payloadLen; i++) {
      payload += _wsBuffer[payloadStart + i];
    }
  }
  
  // 处理不同类型的帧
  switch (opcode) {
    case 0x1: // 文本帧
      logInfo("[WS] 收到文本: " + payload);
      Serial.println("[WebSocket] 收到消息: " + payload); // 直接在串口显示
      if (_dataCallback) {
        _dataCallback(payload);
      }
      break;
    case 0x2: // 二进制帧
      logInfo("[WS] 收到二进制数据，长度: " + String(payloadLen));
      break;
    case 0x8: // 关闭帧
      logInfo("[WS] 收到关闭帧");
      _wsConnected = false;
      break;
    case 0x9: // Ping帧
      logInfo("[WS] 收到Ping，发送Pong");
      sendWebSocketPongFrame("");
      break;
    case 0xA: // Pong帧
      logInfo("[WS] 收到Pong");
      break;
    default:
      logError("[WS] 未知帧类型: " + String(opcode, HEX));
      break;
  }
  
  // 移除已处理的帧
  if (_wsBuffer.length() > headerSize + payloadLen) {
    _wsBuffer = _wsBuffer.substring(headerSize + payloadLen);
    logInfo("[WS] 缓冲区还有数据，继续处理");
    processWebSocketBuffer(); // 递归处理剩余数据
  } else {
    _wsBuffer = "";
  }
}

// 处理WebSocket数据 - 进一步优化误判问题
void LTE4G_Client::processWebSocketData(String data) {
  if (data.length() == 0) return;
  
  logInfo("[WS] 处理新数据，长度: " + String(data.length()) + " 内容: " + data);
  
  // 首先检查是否是WebSocket帧数据 - 这是最重要的判断
  if (data.length() > 0) {
    uint8_t firstByte = (uint8_t)data[0];
    // 检查是否是有效的WebSocket帧标识符
    if (firstByte >= 0x80 && firstByte <= 0x8F) {
      logInfo("[WS] 检测到WebSocket帧数据，直接处理");
      // 这是WebSocket帧数据，绝对不是断开信号
      _wsBuffer += data;
      processWebSocketBuffer();
      return;
    }
  }
  
  // 只有在WebSocket握手尚未完成时，才可能是4G模块的状态信息
  if (!_wsHandshakeComplete) {
    String trimmedData = data;
    trimmedData.trim();
    
    if ((trimmedData == "CLOSED" || trimmedData == "NO CARRIER" || trimmedData == "DISCONNECT") && 
        data.length() < 15) { // 进一步缩小长度限制
      logError("[WS] 握手阶段检测到4G模块连接断开: " + data);
      _wsConnected = false;
      _isConnected = false;
      return;
    }
  }
  
  // WebSocket握手完成后，几乎不应该有纯文本的"CLOSED"
  // 如果WebSocket已连接，"CLOSED"更可能是数据内容
  if (_wsHandshakeComplete && _wsConnected) {
    logInfo("[WS] WebSocket已连接，将数据作为帧内容处理");
    _wsBuffer += data;
    processWebSocketBuffer();
    return;
  }
  
  // 检查明确的网络错误信息（包含多个关键词）
  if ((data.indexOf("ERROR") != -1 && data.indexOf("CONNECT") != -1) ||
      data.indexOf("NO CARRIER") != -1 ||
      data.indexOf("DISCONNECT") != -1) {
    logError("[WS] 检测到网络错误: " + data);
    _wsConnected = false;
    _isConnected = false;
    return;
  }
  
  // 其他情况，将数据添加到缓冲区进行正常处理
  logInfo("[WS] 将数据添加到缓冲区处理");
  _wsBuffer += data;
  processWebSocketBuffer();
}

// 发送WebSocket文本帧 - 修复掩码问题
void LTE4G_Client::sendWebSocketTextFrame(String payload) {
  if (!_wsConnected) {
    logError("[WS] WebSocket未连接，无法发送");
    return;
  }
  
  // 简化的WebSocket帧格式，使用固定掩码0x81
  uint8_t* frame = new uint8_t[payload.length() + 6];
  int frameIndex = 0;
  
  // 第一个字节：FIN=1, Opcode=0001 (文本帧)
  frame[frameIndex++] = 0x81;
  
  // 第二个字节：MASK=1, Payload length (只支持小于126的长度)
  if (payload.length() < 126) {
    frame[frameIndex++] = 0x80 | payload.length();
  } else {
    delete[] frame;
    logError("[WS] 数据太长，不支持发送");
    return;
  }
  
  // 固定掩码（使用0x81的模式）
  uint8_t mask[4] = {0x81, 0x81, 0x81, 0x81};
  for (int i = 0; i < 4; i++) {
    frame[frameIndex++] = mask[i];
  }
  
  // 应用掩码到载荷数据
  for (int i = 0; i < payload.length(); i++) {
    frame[frameIndex++] = payload[i] ^ mask[i % 4];
  }
  
  // 发送帧
  _modemSerial->write(frame, frameIndex);
  logInfo("[WS] 发送文本: " + payload);
  
  delete[] frame;
}

// 发送WebSocket Ping帧 - 修复掩码
void LTE4G_Client::sendWebSocketPing() {
  if (!_wsConnected) return;
  
  // Ping帧格式：0x89 (FIN=1, Opcode=9) + 0x80 (MASK=1, Length=0) + 4字节固定掩码
  uint8_t pingFrame[] = {0x89, 0x80, 0x81, 0x81, 0x81, 0x81};
  _modemSerial->write(pingFrame, 6);
  logInfo("[WS] 发送Ping帧");
}

// 简化的Pong帧发送 - 修复掩码
void LTE4G_Client::sendWebSocketPongFrame(String payload) {
  // 简化实现，直接发送空的Pong帧，使用固定掩码
  uint8_t pongFrame[] = {0x8A, 0x80, 0x81, 0x81, 0x81, 0x81}; // Pong帧 + 固定掩码
  _modemSerial->write(pongFrame, 6);
  logInfo("[WS] 发送Pong帧");
}

// =============== 日志方法 ===============

void LTE4G_Client::logMessage(String message) {
  if (_logCallback) {
    _logCallback(message);
  } else {
    Serial.println("[LTE4G] " + message);
  }
}

void LTE4G_Client::logError(String message) {
  logMessage("[错误] " + message);
}

void LTE4G_Client::logInfo(String message) {
  logMessage("[信息] " + message);
} 