# LTE4G_Library - ESP32 4G模块控制库

一个用于ESP32控制4G LTE模块的Arduino库，支持TCP连接和WebSocket通信。

## 功能特性

- ✅ 自动化4G模块配置流程
- ✅ TCP和WebSocket双模式支持
- ✅ 智能重连机制
- ✅ 回调函数支持
- ✅ 连接状态监控
- ✅ 心跳包机制
- ✅ 详细的日志输出
- ✅ 简洁的API接口

## 硬件要求

- ESP32S3开发板
- 4G LTE模块（支持AT指令）
- SIM卡

## 安装

1. 将以下文件复制到你的Arduino项目目录：
   - `LTE4G_Library.h`
   - `LTE4G_Library.cpp`

2. 在你的Arduino代码中包含头文件：
   ```cpp
   #include "LTE4G_Library.h"
   ```

## 快速开始

### 基本使用

```cpp
#include "LTE4G_Library.h"

// 创建4G客户端对象
LTE4G_Client lte4g(48, 45, 115200); // TX=48, RX=45, 波特率115200

void setup() {
  Serial.begin(115200);
  
  // 初始化4G模块
  if (lte4g.begin("服务器IP", "端口号")) {
    // 配置4G模块
    if (lte4g.configure()) {
      Serial.println("4G模块配置成功！");
    }
  }
}

void loop() {
  // 处理4G模块数据（必须调用）
  lte4g.handleData();
  
  // 发送数据
  if (lte4g.isConnected()) {
    lte4g.sendData("Hello World!");
  }
  
  delay(100);
}
```

### 使用回调函数

```cpp
// 数据接收回调
void onDataReceived(String data) {
  Serial.println("收到: " + data);
}

// 状态变化回调
void onStatusChanged(LTE4G_Status status) {
  if (status == LTE4G_CONNECTED) {
    Serial.println("连接成功！");
  }
}

void setup() {
  // 设置回调函数
  lte4g.setDataCallback(onDataReceived);
  lte4g.setStatusCallback(onStatusChanged);
  
  // 其他初始化代码...
}
```

## API 参考

### 构造函数

```cpp
LTE4G_Client(int txPin = 48, int rxPin = 45, int baudRate = 115200)
```

### 主要方法

#### 初始化和配置

- `bool begin(String serverIP, String serverPort, String apn = "")` - 初始化模块
- `bool configure()` - 配置模块连接
- `void enableWebSocket(bool enable = true)` - 启用WebSocket模式

#### 数据传输

- `bool sendData(String data)` - 发送文本数据
- `bool sendBinaryData(uint8_t* data, size_t length)` - 发送二进制数据
- `void handleData()` - 处理接收数据（必须在loop中调用）

#### 状态监控

- `LTE4G_Status getStatus()` - 获取连接状态
- `bool isConnected()` - 检查是否已连接
- `bool isConfigured()` - 检查是否已配置
- `String getStatistics()` - 获取统计信息

#### 连接管理

- `bool reconnect()` - 重新连接
- `void disconnect()` - 断开连接
- `void reset()` - 重置模块
- `void sendHeartbeat()` - 发送心跳包

#### 信息查询

- `String getModuleInfo()` - 获取模块信息
- `int getSignalStrength()` - 获取信号强度
- `String getIPAddress()` - 获取IP地址
- `String getICCID()` - 获取SIM卡ICCID

#### 回调函数设置

- `void setDataCallback(LTE4G_DataCallback callback)` - 数据接收回调
- `void setStatusCallback(LTE4G_StatusCallback callback)` - 状态变化回调
- `void setLogCallback(LTE4G_LogCallback callback)` - 日志输出回调

### 状态枚举

```cpp
enum LTE4G_Status {
  LTE4G_DISCONNECTED = 0,  // 已断开连接
  LTE4G_CONNECTING,        // 正在连接
  LTE4G_CONNECTED,         // 已连接
  LTE4G_ERROR              // 连接错误
};
```

### 回调函数类型

```cpp
typedef void (*LTE4G_DataCallback)(String data);        // 数据接收回调
typedef void (*LTE4G_StatusCallback)(LTE4G_Status status); // 状态变化回调
typedef void (*LTE4G_LogCallback)(String message);      // 日志输出回调
```

## 配置参数

### 硬件连接

| ESP32引脚 | 4G模块引脚 | 说明 |
|-----------|------------|------|
| GPIO 48   | RX         | ESP32 TX → 4G模块 RX |
| GPIO 45   | TX         | ESP32 RX ← 4G模块 TX |
| GND       | GND        | 共地 |
| 5V        | VCC        | 电源（根据模块要求） |

### 服务器配置

```cpp
const String SERVER_IP = "your_server_ip";     // 服务器IP地址
const String SERVER_PORT = "your_server_port"; // 服务器端口
const String APN = "";                         // APN配置（可选）
```

## 使用示例

详细的使用示例请参考 `example_usage.ino` 文件。

### 串口调试命令

在串口监视器中输入以下命令进行调试：

- `status` - 显示连接状态信息
- `reset` - 重置4G模块
- `disconnect` - 断开连接
- `send <数据>` - 发送自定义数据
- `help` - 显示帮助信息

## 故障排除

### 常见问题

1. **模块无响应**
   - 检查串口连接是否正确
   - 确认波特率设置
   - 检查电源供应

2. **SIM卡无法识别**
   - 确认SIM卡正确插入
   - 检查SIM卡是否欠费
   - 确认APN设置

3. **网络连接失败**
   - 检查信号强度
   - 确认服务器地址和端口
   - 检查防火墙设置

4. **WebSocket握手失败**
   - 确认服务器支持WebSocket
   - 检查握手响应格式
   - 验证服务器配置

### 调试技巧

1. 启用详细日志：
   ```cpp
   lte4g.setLogCallback([](String msg) {
     Serial.println("[调试] " + msg);
   });
   ```

2. 监控连接状态：
   ```cpp
   Serial.println(lte4g.getStatistics());
   ```

3. 检查信号质量：
   ```cpp
   int signal = lte4g.getSignalStrength();
   Serial.println("信号强度: " + String(signal));
   ```

## 技术支持

如有问题或建议，请通过以下方式联系：

- 创建GitHub Issue
- 发送邮件至技术支持

## 版本历史

- **v1.0.0** - 初始版本
  - 基本4G连接功能
  - WebSocket支持
  - 自动重连机制

## 许可证

本项目采用MIT许可证 - 详见LICENSE文件

## 贡献

欢迎提交Pull Request和Issue！

---

© 2024 ESP32 4G Library. 保留所有权利。 