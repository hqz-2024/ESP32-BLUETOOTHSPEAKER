# PCA9554 IO 变化检测程序

## 功能说明

这是一个使用 PCA9554 库的 IO 变化检测程序，具有以下功能：

1. **初始化 PCA9554** - I2C 地址 0x38 (7位)
2. **配置所有 IO 为输入模式** - 使用库函数 `portMode(0xFF)`
3. **中断检测** - 使用 GPIO2 (INT 引脚) 检测 IO 变化
4. **实时输出** - 当检测到 IO 变化时，输出详细的串口信息
5. **状态追踪** - 显示每个引脚的状态变化

## 硬件配置

### I2C 引脚

- **SDA**: GPIO 4
- **SCL**: GPIO 15
- **频率**: 100kHz

### PCA9554 配置

- **I2C 地址**: 0x38 (7位地址)
- **8位写地址**: 0x70
- **8位读地址**: 0x71
- **硬件配置**: A2=A1=A0=0 (全部接地)

### 中断引脚

- **INT 引脚**: GPIO 2
- **触发方式**: 下降沿 (FALLING)
- **上拉**: 内部上拉 (INPUT_PULLUP)

## 使用方法

### 1. 编译上传

```bash
# 编译并上传
pio run -e esp32s3 -t upload

# 或使用其他环境
pio run -e esp32 -t upload
```

### 2. 查看串口输出

```bash
# 打开串口监视器
pio device monitor -b 115200
```

## 输出示例

### 初始化成功

```
╔════════════════════════════════════════╗
║   PCA9554 IO 变化检测程序            ║
╚════════════════════════════════════════╝

🔧 初始化硬件...
✅ I2C 初始化: SDA=4, SCL=15, 频率=100000Hz
✅ INT 引脚配置: GPIO2 (INPUT_PULLUP)

🚀 初始化 PCA9554...
✅ PCA9554 初始化成功！
   7位地址: 0x38
   8位写地址: 0x70
   8位读地址: 0x71
   配置寄存器: 0xFF (11111111) - 所有 IO 为输入

⚙️  设置中断...
✅ 中断已附加到 GPIO2 (下降沿触发)

✅ 中断已启用 (GPIO2)
等待 IO 变化...

初始 IO 状态:
IO 状态: 0xFF (0b11111111)

各引脚状态:
  引脚 | 状态 | 电平
  -----|------|------
  IO0  |  1   | HIGH
  IO1  |  1   | HIGH
  IO2  |  1   | HIGH
  IO3  |  1   | HIGH
  IO4  |  1   | HIGH
  IO5  |  1   | HIGH
  IO6  |  1   | HIGH
  IO7  |  1   | HIGH
```

### IO 变化检测

```
----------------------------------------
时间: 5234 ms
🔔 检测到 IO 变化！
IO 状态: 0xFE (0b11111110)

各引脚状态:
  引脚 | 状态 | 电平
  -----|------|------
  IO0  |  0   | LOW 
  IO1  |  1   | HIGH
  IO2  |  1   | HIGH
  IO3  |  1   | HIGH
  IO4  |  1   | HIGH
  IO5  |  1   | HIGH
  IO6  |  1   | HIGH
  IO7  |  1   | HIGH

变化的引脚:
  IO0: HIGH → LOW 
----------------------------------------
```

## 库函数说明

### PCA9554 类

```cpp
// 初始化
bool begin();

// 配置 IO 模式
bool portMode(byte value);        // 0=输出, 1=输入
bool pinMode(byte pin, bool mode); // 单个引脚配置

// 读取 IO 状态
bool digitalReadPort(byte &value); // 读取所有 IO
bool digitalRead(byte pin, bool &state); // 读取单个 IO

// 写入 IO 状态
bool digitalWritePort(byte value); // 写入所有 IO
bool digitalWrite(byte pin, bool state); // 写入单个 IO

// 极性反转
bool setPortPolarity(byte value);
bool setPinPolarity(byte pin, bool inverted);
```

## 地址说明

⚠️ **重要**：Arduino Wire 库使用 **7 位地址**！

| 格式 | 地址 | 说明 |
|------|------|------|
| 7位地址 | **0x38** | Arduino 代码中使用 ✅ |
| 8位写地址 | 0x70 | 规格书中的写地址 |
| 8位读地址 | 0x71 | 规格书中的读地址 |

### 转换公式

```
7位地址 = 8位地址 >> 1
0x38 = 0x70 >> 1
```

## 中断工作原理

### 中断触发条件

1. **PCA9554 INT 引脚拉低** - 当配置为输入的 IO 引脚状态改变时
2. **ESP32 检测下降沿** - GPIO2 从高变低时触发中断
3. **中断处理函数执行** - 设置 `interruptTriggered` 标志

### 中断复位条件

1. **读取输入寄存器** - 程序调用 `digitalReadPort()` 时自动复位
2. **IO 状态恢复** - 如果 IO 状态恢复到之前的值也会复位

## 故障排除

### 初始化失败

```
❌ PCA9554 初始化失败！
```

**检查项目**：
1. I2C 连接是否正常
2. 设备地址是否正确 (0x38)
3. 上拉电阻是否存在 (4.7kΩ)
4. 电源是否正常

### 中断不触发

**可能原因**：
1. INT 引脚连接不正确
2. 上拉电阻缺失
3. GPIO2 被其他功能占用

**解决方法**：
1. 检查 GPIO2 连接
2. 添加 4.7kΩ 上拉电阻到 VDD
3. 修改 INT_PIN 定义使用其他引脚

### 误触发

**可能原因**：
1. I2C 总线干扰
2. 连接线太长
3. 上拉电阻值不合适

**解决方法**：
1. 缩短连接线
2. 降低 I2C 频率
3. 添加去耦电容

## 代码结构

```
PCA9554-TEST.ino
├── 配置参数
│   ├── I2C 引脚定义
│   ├── PCA9554 地址
│   └── INT 引脚定义
├── 全局变量
│   ├── PCA9554 对象
│   ├── IO 状态
│   └── 中断标志
├── 主程序
│   ├── setup() - 初始化
│   └── loop() - 循环检查
└── 函数实现
    ├── initializeHardware() - 硬件初始化
    ├── initializePCA9554() - PCA9554 初始化
    ├── setupInterrupt() - 中断设置
    ├── handleInterrupt() - 中断处理
    ├── checkIOChanges() - 检查 IO 变化
    ├── printIOState() - 打印状态
    └── printBinary() - 打印二进制
```

## 扩展功能

### 修改为混合模式 (部分输入，部分输出)

```cpp
// 配置 IO0-IO3 为输出，IO4-IO7 为输入
ioExpander.portMode(0xF0);  // 1111 0000
```

### 设置输出电平

```cpp
// 设置 IO0 为 HIGH
ioExpander.digitalWrite(0, HIGH);

// 设置所有输出为 0x0F
ioExpander.digitalWritePort(0x0F);
```

### 启用极性反转

```cpp
// 反转 IO0 的极性
ioExpander.setPinPolarity(0, true);

// 反转所有 IO
ioExpander.setPortPolarity(0xFF);
```

## 参考资料

1. **PCA9554 库** - `arduino_code/libraries2/PCA9554/`
2. **PCA9554 数据手册** - NXP Semiconductors
3. **ESP32 中断** - Espressif Systems
4. **Arduino Wire 库** - https://www.arduino.cc/en/Reference/Wire

## 相关文件

- `PCA9554-TEST.ino` - 主程序
- `README.md` - 本文档
- `arduino_code/libraries2/PCA9554/` - PCA9554 库源码

