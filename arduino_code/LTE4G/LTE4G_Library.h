/**
 * @file LTE4G_Library.h
 * @brief 4G LTE模块控制库
 * @author ESP32 4G Library
 * @version 1.3.0
 * @date 2024
 * @copyright MIT License
 */

#ifndef LTE4G_LIBRARY_H
#define LTE4G_LIBRARY_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <mbedtls/sha1.h>
#include <base64.h>

// 连接状态枚举
enum LTE4G_Status {
  LTE4G_DISCONNECTED = 0,
  LTE4G_CONNECTING,
  LTE4G_CONNECTED,
  LTE4G_ERROR
};

// 配置步骤枚举
enum LTE4G_ConfigStep {
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

// 回调函数类型定义
typedef void (*LTE4G_DataCallback)(String data);
typedef void (*LTE4G_StatusCallback)(LTE4G_Status status);
typedef void (*LTE4G_LogCallback)(String message);

/**
 * @class LTE4G_Client
 * @brief 4G LTE模块客户端类
 */
class LTE4G_Client {
private:
  // 硬件串口
  HardwareSerial* _modemSerial;
  
  // 配置参数
  int _txPin;
  int _rxPin;
  int _baudRate;
  String _serverIP;
  String _serverPort;
  String _apn;
  
  // 状态变量
  LTE4G_Status _status;
  LTE4G_ConfigStep _currentStep;
  bool _isConfigured;
  bool _isConnected;
  bool _isWebSocketMode;
  bool _wsConnected;
  bool _wsHandshakeComplete;
  
  // 重试和超时
  int _stepRetryCount;
  int _globalRetryCount;
  unsigned long _configStartTime;
  unsigned long _stepStartTime;
  unsigned long _lastSendTime;
  unsigned long _lastStatusCheck;
  
  // 缓冲区
  String _modemResponse;
  String _wsBuffer;
  String _wsKey;
  int _sendCounter;
  
  // 回调函数
  LTE4G_DataCallback _dataCallback;
  LTE4G_StatusCallback _statusCallback;
  LTE4G_LogCallback _logCallback;
  
  // 私有方法
  void executeConfigStep();
  void processResponse(String stepName);
  void processConfigResponse();
  String getStepName(LTE4G_ConfigStep step);
  void checkConnectionHealth();
  bool waitForConnect();
  void handleConnectionLoss();
  void handleNetworkLoss();
  void restartConfigFlow();
  
  // WebSocket相关方法
  String generateWebSocketKey();
  String calculateWebSocketAccept(String key);
  void startWebSocketHandshake();
  void checkWebSocketHandshakeResponse();
  void processWebSocketFrames();
  void processWebSocketBuffer();
  void processWebSocketData(String data);
  void processWebSocketFrameData(uint8_t* buffer, int bytesRead);
  void sendWebSocketTextFrame(String payload);
  void sendWebSocketPongFrame(String payload);
  void sendWebSocketPing();
  
  // I2C和寄存器操作
  uint8_t readRegister(uint8_t reg);
  void writeRegister(uint8_t reg, uint8_t value);
  
  // 日志方法
  void logMessage(String message);
  void logError(String message);
  void logInfo(String message);
  
public:
  /**
   * @brief 构造函数
   * @param txPin 串口TX引脚
   * @param rxPin 串口RX引脚
   * @param baudRate 波特率
   */
  LTE4G_Client(int txPin = 48, int rxPin = 45, int baudRate = 921600);
  
  /**
   * @brief 析构函数
   */
  ~LTE4G_Client();
  
  /**
   * @brief 初始化4G模块
   * @param serverIP 服务器IP地址
   * @param serverPort 服务器端口
   * @param apn APN配置（可选）
   * @return 初始化是否成功
   */
  bool begin(String serverIP, String serverPort, String apn = "");
  
  /**
   * @brief 启用WebSocket模式
   * @param enable 是否启用
   */
  void enableWebSocket(bool enable = true);
  
  /**
   * @brief 配置4G模块
   * @return 配置是否成功
   */
  bool configure();
  
  /**
   * @brief 检查连接状态
   * @return 当前连接状态
   */
  LTE4G_Status getStatus();
  
  /**
   * @brief 发送数据
   * @param data 要发送的数据
   * @return 发送是否成功
   */
  bool sendData(String data);
  
  /**
   * @brief 发送二进制数据
   * @param data 数据缓冲区
   * @param length 数据长度
   * @return 发送是否成功
   */
  bool sendBinaryData(uint8_t* data, size_t length);
  
  /**
   * @brief 处理接收数据（需要在loop中调用）
   */
  void handleData();
  
  /**
   * @brief 设置数据接收回调函数
   * @param callback 回调函数
   */
  void setDataCallback(LTE4G_DataCallback callback);
  
  /**
   * @brief 设置状态变化回调函数
   * @param callback 回调函数
   */
  void setStatusCallback(LTE4G_StatusCallback callback);
  
  /**
   * @brief 设置日志回调函数
   * @param callback 回调函数
   */
  void setLogCallback(LTE4G_LogCallback callback);
  
  /**
   * @brief 获取模块信息
   * @return 模块信息字符串
   */
  String getModuleInfo();
  
  /**
   * @brief 获取信号强度
   * @return 信号强度值
   */
  int getSignalStrength();
  
  /**
   * @brief 获取IP地址
   * @return IP地址字符串
   */
  String getIPAddress();
  
  /**
   * @brief 获取SIM卡ICCID
   * @return ICCID字符串
   */
  String getICCID();
  
  /**
   * @brief 断开连接
   */
  void disconnect();
  
  /**
   * @brief 重新连接
   * @return 重连是否成功
   */
  bool reconnect();
  
  /**
   * @brief 发送心跳包
   */
  void sendHeartbeat();
  
  /**
   * @brief 检查是否已连接
   * @return 连接状态
   */
  bool isConnected();
  
  /**
   * @brief 检查是否已配置
   * @return 配置状态
   */
  bool isConfigured();
  
  /**
   * @brief 检查WebSocket是否已连接
   * @return WebSocket连接状态
   */
  bool isWebSocketConnected();
  
  /**
   * @brief 获取统计信息
   * @return 统计信息字符串
   */
  String getStatistics();
  
  /**
   * @brief 重置模块
   */
  void reset();
  
  /**
   * @brief 设置超时时间
   * @param timeout 超时时间（毫秒）
   */
  void setTimeout(unsigned long timeout);
  
  /**
   * @brief 启用调试模式
   * @param enable 是否启用
   */
  void enableDebug(bool enable = true);
};

/**
 * ESP32S3 LTE4G 模块控制库
 * 版本: v1.3.2
 * 作者: AI Assistant
 * 日期: 2024-01-XX
 * 
 * 主要功能:
 * - 自动化4G模块配置（9步配置流程）
 * - TCP和WebSocket双模式支持
 * - 智能重连机制
 * - 回调函数支持
 * - 连接状态监控和心跳机制
 * 
 * v1.3.2 更新内容:
 * - 进一步优化WebSocket连接断开判断逻辑
 * - 优先检查WebSocket帧格式，避免帧数据被误判
 * - 只在握手阶段检查模块状态信息
 * - WebSocket连接后将所有数据作为帧内容处理
 * 
 * v1.3.1 更新内容:
 * - 修复WebSocket连接断开误判问题
 * - 改进连接断开检测逻辑，区分4G模块状态和WebSocket数据
 * - 增加WebSocket帧识别，防止帧数据被误判为断开信号
 * - 优化日志输出，显示具体的数据内容便于调试
 * 
 * v1.3.0 更新内容:
 * - 修复WebSocket握手响应被错误显示的问题
 * - 使用固定掩码0x81，提高兼容性
 * - 改进连接断开检测，防止误判
 * - 优化握手时序，确保TCP连接稳定后再进行WebSocket握手
 * - 增强数据帧处理的稳定性
 */

#endif // LTE4G_LIBRARY_H 