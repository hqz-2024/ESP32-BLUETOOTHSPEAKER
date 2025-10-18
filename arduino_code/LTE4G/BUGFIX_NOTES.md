# LTE4G库修复说明

## v1.3.1 修复 (2024-01-XX)

### 问题描述
用户反馈的问题：
- WebSocket连接经常误判断开，显示"连接已关闭"
- 服务器没有下发数据，但程序仍然判定连接断开
- 从日志看到收到8字节数据就判定为断开连接

### 根本原因分析
**WebSocket连接断开误判的核心问题：**

1. **判断逻辑过于宽泛**：原代码中只要收到包含"CLOSED"的任何数据都会被判定为连接断开
2. **数据来源混乱**：`modemSerial->readString()`读取的数据可能来自：
   - 4G模块的AT指令响应
   - WebSocket协议数据帧
   - 网络层状态信息
   - 实际的用户数据
3. **缺乏上下文判断**：没有区分"CLOSED"是来自哪个协议层
4. **WebSocket帧识别不足**：没有优先识别WebSocket数据帧格式

### 修复措施

#### 1. 改进连接断开判断逻辑
- **严格匹配**：只有短字符串（<20字符）且完全匹配"CLOSED"、"NO CARRIER"、"DISCONNECT"才认为是模块断开
- **上下文检查**：区分4G模块状态信息和WebSocket数据
- **帧识别优先**：优先检查是否为WebSocket帧数据（0x80-0x8F）

#### 2. WebSocket帧数据保护
- **帧标识符检查**：检测0x80-0x8F范围的WebSocket帧标识符
- **数据内容保护**：长度>10的包含"CLOSED"的数据可能是消息内容，不断开连接
- **缓冲区处理**：正确处理WebSocket帧数据到缓冲区

#### 3. 增强日志调试
- **详细内容显示**：日志中显示具体的数据内容，便于问题定位
- **分类处理记录**：记录不同类型数据的处理方式

### 修复后的判断流程
```
收到数据 → 检查长度和内容 → 
├─ 短字符串且匹配断开关键词 → 判定为模块断开
├─ 包含ERROR和CONNECT → 判定为连接错误  
├─ 首字节0x80-0x8F → 识别为WebSocket帧数据
├─ 长数据包含CLOSED → 可能是消息内容，继续处理
└─ 其他数据 → 正常添加到缓冲区处理
```

## v1.3.0 修复 (2024-01-XX)

### 问题描述
用户反馈的问题：
1. 程序刚连上时显示"WebSocket未连接"，发送的信息服务端无法接收解析
2. 接收的第一帧信息无法显示并开始重连
3. 重连后正常收发一段时间，然后又再次重连
4. 掩码字段需要固定为0x81

### 根本原因分析
1. **握手响应处理问题**：WebSocket握手响应被错误地当作模块信息显示，导致握手状态判断错误
2. **掩码兼容性问题**：使用随机掩码可能导致服务器解析困难
3. **连接断开误判**：数据中包含"CLOSED"字符串被误认为连接断开
4. **握手时序问题**：TCP连接建立后立即进行WebSocket握手，可能导致数据混乱

### 修复措施

#### 1. 优化WebSocket握手处理
- **优先检查HTTP响应**：先检查正常的HTTP/1.1 101响应
- **处理握手后数据**：正确处理握手响应后的剩余WebSocket数据帧
- **智能帧检测**：如果错过HTTP响应，通过检测WebSocket帧标识符判断握手成功
- **清空缓冲区**：在握手前清空串口缓冲区，防止数据干扰

#### 2. 修复掩码问题
- **固定掩码**：使用固定的0x81掩码值，提高服务器兼容性
- **统一掩码格式**：所有WebSocket帧（文本、Ping、Pong）都使用相同的固定掩码

#### 3. 改进连接断开检测
- **精确匹配**：只有完全匹配"CLOSED"或以"CLOSED\r"、"CLOSED\n"开头才认为是连接断开
- **排除误判**：防止数据内容中包含"CLOSED"字符串导致的误判

#### 4. 优化握手时序
- **延迟握手**：TCP连接建立后等待1秒再进行WebSocket握手
- **清空缓冲**：握手前清空串口缓冲区和响应缓冲区
- **稳定性保证**：确保TCP连接完全稳定后再进行WebSocket通信

### 代码变更

#### checkWebSocketHandshakeResponse()
```cpp
// 优先检查HTTP响应（正常握手流程）
if (_wsBuffer.indexOf("HTTP/1.1 101") != -1) {
  // 处理握手后的剩余数据
  int headerEnd = _wsBuffer.indexOf("\r\n\r\n");
  if (headerEnd != -1) {
    String remainingData = _wsBuffer.substring(headerEnd + 4);
    if (remainingData.length() > 0) {
      _wsBuffer = remainingData;
      processWebSocketBuffer(); // 立即处理剩余数据
    }
  }
}

// 检查WebSocket数据帧（备用检测）
uint8_t firstByte = (uint8_t)_wsBuffer[0];
if (firstByte == 0x81 || firstByte == 0x82 || firstByte == 0x89 || firstByte == 0x8A) {
  // 握手已成功但错过了HTTP响应
}
```

#### sendWebSocketTextFrame()
```cpp
// 使用固定掩码0x81
uint8_t mask[4] = {0x81, 0x81, 0x81, 0x81};
```

#### waitForConnect()
```cpp
// 清空响应缓冲区，防止干扰WebSocket握手
_modemResponse = "";

// 等待连接稳定
delay(1000);

// 清空串口缓冲区
while (_modemSerial->available()) {
  _modemSerial->read();
}
```

#### processWebSocketData()
```cpp
// 精确的连接断开检测
if (data == "CLOSED" || data.startsWith("CLOSED\r") || data.startsWith("CLOSED\n")) {
  // 真正的连接断开
}
```

### 预期效果
1. **握手稳定性**：WebSocket握手成功率提升，不再出现握手响应显示错误
2. **数据兼容性**：使用固定掩码提高与服务器的兼容性
3. **连接稳定性**：减少误判导致的频繁重连
4. **时序优化**：握手时序更加合理，避免数据混乱

### 测试建议
1. 测试初始连接：确认WebSocket状态正确显示
2. 测试数据收发：验证服务器能正确接收和解析数据
3. 测试长时间运行：确认连接稳定性
4. 测试重连机制：验证真正断开时的重连功能 