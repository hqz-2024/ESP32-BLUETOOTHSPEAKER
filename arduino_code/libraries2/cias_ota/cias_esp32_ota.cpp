#include "cias_esp32_ota.h"
// 在文件开头添加必要的头文件
#include "cias_crc.h"
#include <FFat.h>             // 使用FFat库来访问ESP32内部分区
#include <HTTPClient.h>       // 添加HTTPClient库
#include <WiFiClientSecure.h> // 添加WiFiClientSecure库以支持HTTPS
#include <WiFiClient.h>       // 添加WiFiClient库

// 初始化静态成员变量
CIAS_ESP32_OTA *CIAS_ESP32_OTA::instance = nullptr;

// 初始化 onStartCallback
void (*CIAS_ESP32_OTA::onStartCallback)(void) = []()
{
    if (CIAS_ESP32_OTA::instance)
    {
        CIAS_ESP32_OTA::instance->startUpdateTime = millis();
        CIAS_ESP32_OTA::instance->startUpdateEd = true;
        Serial.println("CALLBACK: HTTP更新进程启动");
    }
};

// 初始化 onEndCallback
void (*CIAS_ESP32_OTA::onEndCallback)(void) = []()
{
    if (CIAS_ESP32_OTA::instance)
    {
        Serial.println("[Info] -> CALLBACK: HTTP更新进程完成");
    }
};

// 初始化 onProgressCallback
void (*CIAS_ESP32_OTA::onProgressCallback)(int percent) = [](int percent)
{
    if (CIAS_ESP32_OTA::instance)
    {
        CIAS_ESP32_OTA::instance->isUpdateProgress = true;
        CIAS_ESP32_OTA::instance->progressPercent = percent;

        // 直接使用progressPercent，不使用不存在的otaProgress
        String progress = String(percent) + "%";

        // 使用类内部的onProgress函数指针
        if (CIAS_ESP32_OTA::instance->onProgress)
        {
            CIAS_ESP32_OTA::instance->onProgress(percent);
        }

        // 使用静态局部变量代替不存在的prevSendProgressTime
        static unsigned long lastLogTime = 0;
        if (millis() - lastLogTime > 1500)
        {
            Serial.printf("[Info] -> 更新进度：%s", progress.c_str());
            lastLogTime = millis();

            // JSONVar data_ota;
            // data_ota["type"] = "ota_progress";
            // data_ota["data"] = percent;
            // data_ota["device_id"] = CIAS_ESP32_OTA::instance->deviceId;
            // String sendData = JSON.stringify(data_ota);
            // CIAS_ESP32_OTA::instance->webSocket->sendTXT(sendData);
            if (CIAS_ESP32_OTA::instance->ledAmi)
            {
                CIAS_ESP32_OTA::instance->ledAmi();
            }
        }
    }
};

// 初始化 onErrorCallback
void (*CIAS_ESP32_OTA::onErrorCallback)(int errorCode) = [](int errorCode)
{
    if (CIAS_ESP32_OTA::instance)
    {
        Serial.printf("CALLBACK: HTTP更新致命错误代码 %d\n", errorCode);
    }
};

// 实现带全部参数的构造函数
CIAS_ESP32_OTA::CIAS_ESP32_OTA(
    const String &device_id,
    HardwareSerial *serial,
    WebSocketsClient *webSocket,
    void (*ShowNotification)(const char *data),
    void (*OnlyShowNotification)(bool only_show_notification),
    void (*onProgress)(int percent),
    void (*stopSessionCb)(),
    void (*delAllTaskCb)(),
    void (*awaitPlayerDoneCb)(),
    void (*playBuiltinAudioCb)(const unsigned char *data, size_t len),
    void (*ledAmiCb)())
    : device_id(device_id), // 在初始化列表中初始化引用成员
      serialPort(serial)
{ // 初始化serialPort
    uartQueue = NULL;
    uartReadTaskHandle = NULL;

    // 初始化状态变量
    startUpdateTime = 0;
    startUpdateEd = false;
    isUpdateProgress = false;
    willUpdate = false;
    progressPercent = 0;

    // 设置instance指针
    instance = this;
    // 初始化从ota_manager参考的回调函数指针
    this->webSocket = webSocket;
    this->showNotification = ShowNotification;
    this->onlyShowNotification = OnlyShowNotification;
    this->onProgress = onProgress;
    this->stopSession = stopSessionCb;
    this->delAllTask = delAllTaskCb;
    this->awaitPlayerDone = awaitPlayerDoneCb;
    this->playBuiltinAudio = playBuiltinAudioCb;
    this->ledAmi = ledAmiCb;
}

CIAS_ESP32_OTA::~CIAS_ESP32_OTA()
{
    end();
}

// 修改begin函数中的波特率初始化为115200等发送完enterOtaMode后再改为921600
bool CIAS_ESP32_OTA::begin()
{
    // 初始化串口
    Serial.println("初始化串口");
    serialPort->begin(115200, SERIAL_8N1, esp_ai_serial_rx, esp_ai_serial_tx);
    Serial.println("初始化完毕串口");
    // 创建队列
    Serial.println("创建队列");
    uartQueue = xQueueCreate(10, sizeof(uart_msg_t));
    if (uartQueue == NULL)
    {
        Serial.println("创建队列失败");
        return false;
    }
    Serial.println("创建队列完成");
    // 创建串口读取任务
    if (xTaskCreate(uartReadTask, "uartReadTask", 2048, this, 1, &uartReadTaskHandle) != pdPASS)
    {
        Serial.println("创建任务失败");
        vQueueDelete(uartQueue);
        uartQueue = NULL;
        return false;
    }
    Serial.println("初始化OTA包");
    // 初始化OTA包
    ciasOtaDataInit();

    // 重置状态
    willUpdate = false;
    startUpdateEd = false;
    isUpdateProgress = false;
    progressPercent = 0;

    return true;
}

void CIAS_ESP32_OTA::end()
{
    // 1. 首先停止任务
    if (uartReadTaskHandle != NULL)
    {
        vTaskDelete(uartReadTaskHandle);
        uartReadTaskHandle = NULL;
        // 给任务一点时间完全终止
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    // 2. 然后删除队列
    if (uartQueue != NULL)
    {
        vQueueDelete(uartQueue);
        uartQueue = NULL;
    }

    // 3. 关闭文件
    if (otaFile)
    {
        otaFile.close();
    }
    Serial.println("结束OTA包");
    // 4. 注意：不再在这里关闭串口，避免与外部调用冲突
    // 串口应该由调用者负责管理开关
}

/*
// 回调函数设置方法已移除，回调函数已在静态成员变量定义时直接初始化
void CIAS_ESP32_OTA::onStart(void (*callback)(void)) {
    onStartCallback = callback;
}

void CIAS_ESP32_OTA::onEnd(void (*callback)(void)) {
    onEndCallback = callback;
}

void CIAS_ESP32_OTA::onProgress(void (*callback)(int percent)) {
    onProgressCallback = callback;
}

void CIAS_ESP32_OTA::onError(void (*callback)(int errorCode)) {
    onErrorCallback = callback;
}
*/

// 状态查询方法
bool CIAS_ESP32_OTA::isUpdating() const
{
    if (willUpdate)
    {
        return true;
    }
    return startUpdateEd && !updateFailed();
}

int CIAS_ESP32_OTA::getProgress() const
{
    return progressPercent;
}

bool CIAS_ESP32_OTA::updateFailed() const
{
    // 如果长时间没有进度更新，认为升级失败
    if (startUpdateEd && !isUpdateProgress && (millis() - startUpdateTime > 60000))
    { // 60秒超时
        return true;
    }
    return false;
}

// 实现reset方法
void CIAS_ESP32_OTA::reset()
{
    // 1. 清理现有资源
    end();

    // 2. 重置成员变量到初始状态
    uartQueue = NULL;
    uartReadTaskHandle = NULL;
    fileSize = 0;
    pkgCount = 0;

    // 3. 重新初始化OTA包数据
    ciasOtaDataInit();

    // 添加一个小延时，确保所有资源都已释放
    vTaskDelay(pdMS_TO_TICKS(100));
}

uint16_t CIAS_ESP32_OTA::crc16_ccitt(uint16_t preCrc, const uint8_t *data, uint32_t length)
{
    // 直接使用已有的CRC实现
    return ::crc16_ccitt(preCrc, data, length);
}

uint16_t CIAS_ESP32_OTA::getOtaPackPayloadLen(cias_ota_pack_t *otaPack)
{
    uint16_t payloadLen = otaPack->len0 * 256 + otaPack->len1 - 4;
    return payloadLen;
}

void CIAS_ESP32_OTA::ciasOtaDataInit(void)
{
    otaPack.head0 = MSG_HEAD_HIGH;
    otaPack.head1 = MSG_HEAD_LOW;
    otaPack.tail = MSG_TAIL;
    otaPack.data = otaPayloadData;
}

void CIAS_ESP32_OTA::ciasOtaDataCheckVersion()
{
    otaPack.len0 = 0;
    otaPack.len1 = 4;
    otaPack.msg_type = MSG_TYPE_OTA_VERSION;
    uint16_t crc = crc16_ccitt(0, (uint8_t *)&otaPack, 5);
    otaPack.crc0 = crc / 256;
    otaPack.crc1 = crc % 256;
}

// 在ciasOtaDataStart函数中修改文件打开方式
bool CIAS_ESP32_OTA::ciasOtaDataStart(const char *otaFileName)
{
    uint8_t version[4];

    // 将SD.open改为FFat.open
    otaFile = FFat.open(otaFileName, "r");
    if (!otaFile)
    {
        Serial.printf("升级固件打开失败: %s\r\n", otaFileName);
        return false;
    }

    // 获取文件大小
    fileSize = otaFile.size();
    fileSize -= OTA_INVALID_HEAD_LENGTH;
    pkgCount = fileSize / OTA_PACK_LENGTH;
    if (fileSize % OTA_PACK_LENGTH)
    {
        pkgCount++;
    }

    // 定位到版本号位置
    if (!otaFile.seek(OTA_INVALID_HEAD_LENGTH + 0x90))
    {
        otaFile.close();
        return false;
    }

    // 读取版本号
    int readLen = otaFile.read(version, 4);
    if (readLen != 4)
    {
        otaFile.close();
        return false;
    }

    Serial.printf("待升级固件大小:0x%x, 包数:%d; 版本号 %d.%d.%d\r\n",
                  fileSize, pkgCount, version[2], version[1], version[0]);

    // 构造OTA开始包
    otaPack.len0 = 0;
    otaPack.len1 = 11;
    otaPack.msg_type = MSG_TYPE_OTA_START;
    memset(otaPayloadData, 0, OTA_PACK_LENGTH + 2);
    otaPayloadData[0] = version[2];
    otaPayloadData[1] = version[1];
    otaPayloadData[2] = version[0];
    otaPayloadData[3] = pkgCount / 256;
    otaPayloadData[4] = pkgCount % 256;
    otaPayloadData[5] = OTA_PACK_LENGTH / 256;
    otaPayloadData[6] = OTA_PACK_LENGTH % 256;

    // 计算CRC
    uint16_t crc = crc16_ccitt(0, (uint8_t *)&otaPack, 5);
    crc = crc16_ccitt(crc, otaPayloadData, 7);
    otaPack.crc0 = crc / 256;
    otaPack.crc1 = crc % 256;

    return true;
}

bool CIAS_ESP32_OTA::ciasOtaDataFirmware(uint16_t packCount)
{
    int readLen = 0;

    otaPack.msg_type = MSG_TYPE_OTA_FIRMWARE;
    memset(otaPayloadData, 0, OTA_PACK_LENGTH + 2);
    otaPayloadData[0] = packCount / 256;
    otaPayloadData[1] = packCount % 256;

    // 定位到文件位置
    if (!otaFile.seek(packCount * OTA_PACK_LENGTH + OTA_INVALID_HEAD_LENGTH))
    {
        return false;
    }

    // 读取数据
    readLen = otaFile.read(&otaPayloadData[2], OTA_PACK_LENGTH);
    if (readLen <= 0)
    {
        return false;
    }

    // 设置长度和CRC
    otaPack.len0 = (readLen + 6) / 256;
    otaPack.len1 = (readLen + 6) % 256;
    uint16_t crc = crc16_ccitt(0, (uint8_t *)&otaPack, 5);
    crc = crc16_ccitt(crc, otaPayloadData, readLen + 2);
    otaPack.crc0 = crc / 256;
    otaPack.crc1 = crc % 256; // 分两段计算crc,并转换为小端模式

    return true;
}

void CIAS_ESP32_OTA::ciasOtaDataFinish()
{
    otaPack.len0 = 0;
    otaPack.len1 = 4;
    otaPack.msg_type = MSG_TYPE_OTA_FINISH;
    uint16_t crc = crc16_ccitt(0, (uint8_t *)&otaPack, 5);
    otaPack.crc0 = crc / 256;
    otaPack.crc1 = crc % 256;
}

int CIAS_ESP32_OTA::sendOtaPackage()
{
    // Serial.printf("send_ota_package:\r\n");
    int payloadLen = otaPack.len0 * 256 + otaPack.len1 - 4;
    // Serial.printf("payloadLen: %d\r\n", payloadLen);
    memset(otaUartSendData, 0, OTA_PACK_LENGTH + 8);
    memcpy(otaUartSendData, &otaPack, 5);
    // Serial.printf("memcpy success\r\n");
    if (payloadLen != 0)
    {
        memcpy(&otaUartSendData[5], otaPack.data, payloadLen);
    }
    memcpy(&otaUartSendData[5 + payloadLen], (uint8_t *)&otaPack.crc0, 3);

    // Serial.print("Send HEX: ");
    // for (int i = 0; i < payloadLen + 8; i++) {
    //     Serial.printf("%02X ", otaUartSendData[i]);
    // }
    // Serial.println();

    // 通过串口发送数据
    serialPort->write(otaUartSendData, payloadLen + 8);
    // serialPort->flush();

    return 0;
}

bool CIAS_ESP32_OTA::verifyOtaAck(uint8_t *data, const uint8_t recvMsgType)
{
    if ((data[0] != MSG_HEAD_HIGH) || (data[1] != MSG_HEAD_LOW))
    {
        return false;
    }

    if (data[4] != recvMsgType)
    {
        Serial.printf("ota_ack type error, expected: %d, got: %d\r\n", recvMsgType, data[4]);
        return false;
    }

    uint16_t crc = crc16_ccitt(0, data, data[3] + 1);
    data += (data[3] + 1);
    if ((data[0] != crc / 256) || (data[1] != crc % 256))
    {
        Serial.printf("verify_ota_ack failed: %x vs %x\r\n", crc, ((uint16_t *)data)[0]);
        return false;
    }

    return true;
}

bool CIAS_ESP32_OTA::recvOtaAck(int timeoutMs, const uint8_t recvMsgType, uint8_t *recvPayloadData)
{
    int timeCount = 0;
    uart_msg_t uartMsg;
    int readLen = 0;

    // 清空接收队列中的历史数据
    while (xQueueReceive(uartQueue, &uartMsg, 0) == pdTRUE)
        ;

    while (1)
    {
        // 非阻塞方式接收队列消息
        readLen = xQueueReceive(uartQueue, &uartMsg, pdMS_TO_TICKS(1));

        if (readLen == pdTRUE)
        {
            // 收到了消息，进行处理
            // Serial.printf("recv ack:");
            // for (int i = 0; i < UART_RCV_BUF_SIZE && uartMsg.mtext[i] != 0; i++) {
            //     Serial.printf("%02x ", (uint8_t)uartMsg.mtext[i]);
            // }
            // Serial.printf("\r\n");

            // 验证ACK是否匹配
            if (verifyOtaAck((uint8_t *)uartMsg.mtext, recvMsgType))
            {
                // 提取payload数据
                int payloadLen = ((uint8_t *)uartMsg.mtext)[3] - 4;
                memcpy(recvPayloadData, &uartMsg.mtext[5], payloadLen);
                return true;
            }
        }
        else
        {
            // 没有收到消息，短暂休眠并检查超时
            vTaskDelay(pdMS_TO_TICKS(1)); // 休眠1ms
            if (timeCount++ > timeoutMs)
            {
                Serial.printf("recv_package error\r\n");
                return false;
            }
        }
    }
}

bool CIAS_ESP32_OTA::recvEnterOtaModeAck(int timeoutMs)
{
    int timeCount = 0;
    uart_msg_t uartMsg;

    while (timeCount < timeoutMs)
    {
        if (xQueueReceive(uartQueue, &uartMsg, pdMS_TO_TICKS(10)))
        {
            Serial.printf("recv data:");
            for (int i = 0; i < UART_RCV_BUF_SIZE && uartMsg.mtext[i] != 0; i++)
            {
                Serial.printf("%02x ", (uint8_t)uartMsg.mtext[i]);
            }
            Serial.printf("\r\n");

            if (uartMsg.mtext[5] == 0x97)
            {
                Serial.printf("recv_enter_ota_mode_ack\r\n");
                return true;
            }
        }
        delay(1);
        timeCount++;
    }

    Serial.printf("recv_package error: timeout\r\n");
    return false;
}

void CIAS_ESP32_OTA::uartReadTask(void *pvParameters)
{
    CIAS_ESP32_OTA *instance = static_cast<CIAS_ESP32_OTA *>(pvParameters);
    uart_msg_t uartMsg;
    uint8_t recvBuffer[UART_RCV_BUF_SIZE];
    int recvLen = 0;
    int bufferPos = 0; // 添加缓冲区位置指针，用于累积不完整的帧

    while (true)
    {
        // 添加检查：如果串口不可用，短暂延迟后继续循环
        if (!instance->serialPort)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (instance->serialPort->available())
        {
            // 读取可用数据，但不填满整个缓冲区
            int availableBytes = instance->serialPort->available();
            int readBytes = min(availableBytes, UART_RCV_BUF_SIZE - bufferPos);
            recvLen = instance->serialPort->readBytes(&recvBuffer[bufferPos], readBytes);

            if (recvLen > 0)
            {
                bufferPos += recvLen;

                // 查找完整包（以0xff或0xfb结尾）
                int packetStart = -1;
                int packetEnd = -1;

                // 查找消息头开始（0xA5）
                for (int i = 0; i < bufferPos - 1; i++)
                {
                    if (recvBuffer[i] == MSG_HEAD_HIGH && recvBuffer[i + 1] == MSG_HEAD_LOW)
                    {
                        packetStart = i;
                        break;
                    }
                }

                // 如果找到了消息头，查找包的结尾
                if (packetStart >= 0)
                {
                    // 改进：查找最后一个0xFF或0xFB作为包的结束
                    for (int i = packetStart; i < bufferPos; i++)
                    {
                        if (recvBuffer[i] == 0xff || recvBuffer[i] == 0xfb)
                        {
                            packetEnd = i;
                        }
                    }
                    // 或者，根据逻辑分析仪中的模式，可能需要特殊处理连续的结束字符
                    // 例如：检查是否有连续的0xFF序列
                }

                // 如果找到了完整的包
                if (packetStart >= 0 && packetEnd >= 0)
                {
                    int packetLength = packetEnd - packetStart + 1;

                    // // 打印接收到的完整应答帧
                    // Serial.printf("Received frame: ");
                    // for (int i = packetStart; i <= packetEnd; i++)
                    // {
                    //     Serial.printf("%02x ", recvBuffer[i]);
                    // }
                    // Serial.printf("\n");

                    // 处理接收到的包
                    uartMsg.mtype = 1;
                    memset(uartMsg.mtext, 0, UART_RCV_BUF_SIZE);
                    memcpy(uartMsg.mtext, &recvBuffer[packetStart], packetLength);

                    // 将剩余数据移到缓冲区开头
                    int remainingBytes = bufferPos - packetEnd - 1;
                    if (remainingBytes > 0)
                    {
                        memmove(recvBuffer, &recvBuffer[packetEnd + 1], remainingBytes);
                    }
                    bufferPos = remainingBytes;

                    // 继续处理
                    xQueueSend(instance->uartQueue, &uartMsg, portMAX_DELAY);
                }

                // 如果缓冲区已满但没有找到完整包，重置缓冲区
                if (bufferPos >= UART_RCV_BUF_SIZE - 1)
                {
                    bufferPos = 0;
                }
            }
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

// 修改enterOtaMode函数，在发送完指令后将波特率改为3M
bool CIAS_ESP32_OTA::enterOtaMode()
{
    // AIOT SDK进入OTA标准协议
    uint8_t recvPayloadData[10];
    // uint8_t data[16] = {0xa5, 0xa5, 0x5a, 0x5a, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12};
    uint8_t data[10] = {0xA5, 0xFC, 0x00, 0x00, 0xA1, 0x97, 0x00, 0x38, 0x01, 0xFB};
    serialPort->write(data, 10);
    serialPort->flush();
    serialPort->updateBaudRate(921600);
    return true;
}

bool CIAS_ESP32_OTA::otaCheckVersion()
{
    uint8_t recvPayloadData[10];
    ciasOtaDataCheckVersion();
    sendOtaPackage();

    if (recvOtaAck(500, MSG_TYPE_OTA_VERSION, recvPayloadData))
    {
        Serial.printf("语音芯片软件版本号: %d.%d.%d; 硬件版本号: %d.%d.%d\r\n",
                      recvPayloadData[0], recvPayloadData[1], recvPayloadData[2],
                      recvPayloadData[5], recvPayloadData[4], recvPayloadData[3]);
        return true;
    }

    return false;
}

bool CIAS_ESP32_OTA::otaStart(const char *otaFileName)
{
    uint8_t recvPayloadData[10];

    if (!ciasOtaDataStart(otaFileName))
    {
        return false;
    }

    int retry = 10;
    while (retry--)
    {
        sendOtaPackage();
        if (recvOtaAck(5000, MSG_TYPE_OTA_START, recvPayloadData))
        {
            return true;
        }
    }

    // 失败时关闭文件
    if (otaFile)
    {
        otaFile.close();
    }

    return false;
}

bool CIAS_ESP32_OTA::otaTransportFirmware()
{
    uint8_t recvPayloadData[10];
    uint16_t packCurrent = 0;
    unsigned long totalTime = 0; // 总传输时间
    int packetCount = 0;         // 成功传输的包数量

    while (packCurrent < pkgCount)
    {
        Serial.printf("ota_transport_firmware %d\r\n", packCurrent);

        if (!ciasOtaDataFirmware(packCurrent))
        {
            return false;
        }

        Serial.printf("ota_data firmware success\n");

        // 记录发送前的时间戳（微秒）
        unsigned long startTime = micros();

        // 发送数据包
        sendOtaPackage();

        // 接收ACK
        if (!recvOtaAck(5000, MSG_TYPE_OTA_FIRMWARE, recvPayloadData))
        {
            // 重试一次
            Serial.printf("第一次接收ACK超时，重试一次\r\n");
            sendOtaPackage();
            if (!recvOtaAck(3000, MSG_TYPE_OTA_FIRMWARE, recvPayloadData))
            {
                Serial.printf("OTA ack 返回超时\r\n");
                return false;
            }
        }

        // 记录接收ACK后的时间戳并计算时间差
        unsigned long endTime = micros();
        unsigned long transferTime = endTime - startTime;
        totalTime += transferTime;
        packetCount++;

        // 打印这包数据的传输时间（毫秒）
        Serial.printf("包 %d 传输时间: %.3f ms\r\n", packCurrent, transferTime / 1000.0);

        // 以下是原有的校验代码
        if (recvPayloadData[0] == 1)
        {
            if ((recvPayloadData[1] != packCurrent / 256) || (recvPayloadData[2] != packCurrent % 256))
            {
                Serial.printf("OTA ack 返回的包序号不正确\r\n");
                return false;
            }
        }
        else if (recvPayloadData[0] == 0)
        {
            // 错误处理代码保持不变
            if ((int8_t)recvPayloadData[1] == -1)
            {
                Serial.printf("语音芯片端内存不足，不能ota\r\n");
                return false;
            }
            else if ((int8_t)recvPayloadData[1] == -2)
            {
                Serial.printf("传输的固件不是ota打包的固件\r\n");
                return false;
            }
            else if ((int8_t)recvPayloadData[1] == -3)
            {
                Serial.printf("传输的固件分区表信息校验错误\r\n");
                return false;
            }
            else if ((int8_t)recvPayloadData[1] == -4)
            {
                Serial.printf("传输的固件硬件版本号不一致\r\n");
                return false;
            }
            else
            {
                Serial.printf("OTA ACK返回其他错误 %d\r\n", (int8_t)recvPayloadData[1]);
                return false;
            }
        }

        // 传输成功一包后更新状态
        isUpdateProgress = true;

        // 获取下一包序号
        packCurrent = recvPayloadData[3] * 256 + recvPayloadData[4];
        Serial.printf("recv next count %d %d\r\n", packCurrent, pkgCount);

        if (packCurrent >= pkgCount)
        { // 语音端确认收到了最后一包，返回成功
            Serial.printf("last frame %d %d\r\n", packCurrent, pkgCount);

            // 打印平均传输时间
            if (packetCount > 0)
            {
                float avgTime = totalTime / (float)packetCount / 1000.0; // 转换为毫秒
                Serial.printf("总共传输 %d 包，平均每包传输时间: %.3f ms\r\n", packetCount, avgTime);
            }
            return true;
        }

        // vTaskDelay(pdMS_TO_TICKS(1)); // 保留注释，方便调试时使用
    }

    return false;
}

bool CIAS_ESP32_OTA::otaFinish()
{
    uint8_t recvPayloadData[10];
    ciasOtaDataFinish();
    sendOtaPackage();

    if (recvOtaAck(1000, MSG_TYPE_OTA_FINISH, recvPayloadData))
    {
        if (recvPayloadData[0] == 1)
        {
            // 关闭文件
            Serial.printf("ota_finish success\r\n");
            return true;
        }
    }

    return false;
}

// 从URL直接流式传输固件数据到语音芯片
// 从URL直接流式传输固件数据到语音芯片
bool CIAS_ESP32_OTA::otaFromUrlStream(const char *firmwareUrl)
{
    Serial.printf("固件URL: %s\r\n", firmwareUrl);

    // 重置状态
    startUpdateTime = millis();
    startUpdateEd = false;
    isUpdateProgress = false;
    willUpdate = true;
    progressPercent = 0;

    // 调用停止会话和删除所有任务的回调
    if (this->stopSession)
    {
        this->stopSession();
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);

    if (this->delAllTask)
    {
        this->delAllTask();
    }

    // 显示通知
    if (this->showNotification)
    {
        this->showNotification("正在更新唤醒词");
    }

    if (this->onlyShowNotification)
    {
        this->onlyShowNotification(true);
    }

    // 初始化WiFiClient用于HTTP请求
    WiFiClient client;
    HTTPClient http;

    // 开始HTTP请求
    if (!http.begin(client, firmwareUrl))
    {
        Serial.println("无法连接到服务器");
        if (onErrorCallback)
            onErrorCallback(2); // 错误码2: HTTP连接失败
        return false;
    }

    // 添加必要的请求头
    http.addHeader("User-Agent", "ESP32-OTA-Updater");
    http.addHeader("Accept", "*/*");

    // 发送GET请求
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP请求失败，错误码: %d\r\n", httpCode);

        // 获取更多错误信息
        if (httpCode > 0)
        {
            String response = http.getString();
            Serial.printf("服务器响应: %s\r\n", response.c_str());
        }
        else
        {
            Serial.printf("连接错误: %s\r\n", http.errorToString(httpCode).c_str());
        }

        http.end();
        if (onErrorCallback)
            onErrorCallback(2); // 错误码2: HTTP连接失败
        return false;
    }

    // 获取文件大小
    int contentLength = http.getSize();
    Serial.printf("固件原始大小: %d 字节\r\n", contentLength);

    // 计算有效固件大小（减去无效头部）
    int effectiveFirmwareSize = contentLength - OTA_INVALID_HEAD_LENGTH;
    Serial.printf("固件有效大小:0x%x 字节\r\n", effectiveFirmwareSize);

    // 计算总包数
    pkgCount = (effectiveFirmwareSize + OTA_PACK_LENGTH - 1) / OTA_PACK_LENGTH;

    // 获取HTTP流
    WiFiClient *stream = http.getStreamPtr();

    // 使用小缓冲区逐包读取，避免堆栈溢出
    uint8_t smallBuffer[OTA_PACK_LENGTH]; // 1024字节的小缓冲区
    uint8_t version[4] = {0};             // 存储版本信息(使用4字节数组以支持完整版本信息)
    bool versionExtracted = false;        // 版本信息是否已提取

    // 初始化OTA包
    ciasOtaDataInit();

    // // 进入OTA模式
    // if (!enterOtaMode())
    // {
    //     Serial.println("进入OTA模式失败");
    //     if (onErrorCallback)
    //         onErrorCallback(1); // 错误码1: 进入OTA模式失败
    //     http.end();
    //     return false;
    // }
    // 调用onStart回调
    if (onStartCallback)
        onStartCallback();
    startUpdateTime = millis();
    startUpdateEd = true;

    // 延时确保进入OTA模式
    delay(1000);

    // // 检查版本
    // if (!otaCheckVersion())
    // {
    //     Serial.println("版本检查失败");
    //     if (onErrorCallback)
    //         onErrorCallback(7); // 错误码7: 版本检查失败
    //     http.end();
    //     return false;
    // }

    // 提取版本信息
    Serial.println("正在跳过无效的固件头部数据...");
    const uint32_t VERSION_OFFSET = OTA_INVALID_HEAD_LENGTH + 0x90; // 版本信息偏移量
    uint32_t skippedBytes = 0;

    // 创建一个临时缓冲区专门用于存储版本信息区域
    uint8_t versionAreaBuffer[256]; // 足够存储版本信息附近的数据
    uint32_t versionAreaPos = 0;

    // 跳过无效头部并尝试提取版本信息
    while (skippedBytes < OTA_INVALID_HEAD_LENGTH)
    {
        size_t bytesRead = stream->readBytes(smallBuffer, min((size_t)(OTA_INVALID_HEAD_LENGTH - skippedBytes), sizeof(smallBuffer)));
        if (bytesRead == 0)
        {
            Serial.println("错误: 读取固件数据失败");
            if (onErrorCallback)
                onErrorCallback(3); // 错误码3: 固件下载失败
            http.end();
            return false;
        }

        // 保存版本信息区域数据
        if (skippedBytes < VERSION_OFFSET + 128)
        {
            // 只保存可能包含版本信息的区域
            size_t copyStart = max(0, (int)(VERSION_OFFSET - 64 - skippedBytes));
            // 通过类型转换修复类型不匹配问题
            size_t copyLen = min(bytesRead - copyStart, sizeof(versionAreaBuffer) - (size_t)versionAreaPos);
            if (copyLen > 0 && copyStart < bytesRead)
            {
                memcpy(versionAreaBuffer + versionAreaPos, smallBuffer + copyStart, copyLen);
                versionAreaPos += copyLen;
            }
        }

        skippedBytes += bytesRead;
    }

    Serial.println("跳过无效头部完成，开始提取版本信息...");

    // 模拟ciasOtaDataStart中的版本信息提取逻辑

    // 计算版本信息在缓冲区中的偏移量
    uint32_t versionInBufferOffset = 64; // 因为我们从VERSION_OFFSET - 64开始保存

    if (versionInBufferOffset + 4 <= versionAreaPos)
    {
        // 找到了有效位置，读取版本信息
        memcpy(version, versionAreaBuffer + versionInBufferOffset, 4);

        // 检查版本号是否合理
        if (version[0] <= 255 && version[1] <= 255 && version[2] <= 255)
        {
            versionExtracted = true;
            Serial.printf("成功提取到版本信息: %d.%d.%d\n", version[2], version[1], version[0]);
        }
    }

    // 如果没有提取到有效的版本信息，使用默认版本
    if (!versionExtracted)
    {
        version[0] = 1; // 默认版本号 2.0.1
        version[1] = 0;
        version[2] = 2;
        Serial.println("版本信息提取失败，使用默认版本: 2.0.1");
    }

    Serial.printf("待升级固件大小:0x%x, 包数:%d; 版本号 %d.%d.%d\n",
                  effectiveFirmwareSize, pkgCount, version[2], version[1], version[0]);
    versionExtracted = true;

    // 构造OTA开始包
    otaPack.len0 = 0;
    otaPack.len1 = 11;
    otaPack.msg_type = MSG_TYPE_OTA_START;
    memset(otaPayloadData, 0, OTA_PACK_LENGTH + 2);
    otaPayloadData[0] = version[2];
    otaPayloadData[1] = version[1];
    otaPayloadData[2] = version[0];
    otaPayloadData[3] = pkgCount / 256;
    otaPayloadData[4] = pkgCount % 256;
    otaPayloadData[5] = OTA_PACK_LENGTH / 256;
    otaPayloadData[6] = OTA_PACK_LENGTH % 256;

    // 计算CRC
    uint16_t crc = crc16_ccitt(0, (uint8_t *)&otaPack, 5);
    crc = crc16_ccitt(crc, otaPayloadData, 7);
    otaPack.crc0 = crc / 256;
    otaPack.crc1 = crc % 256;

    // 发送OTA开始包
    uint8_t recvPayloadData[10];
    int retry = 10;
    while (retry--)
    {
        sendOtaPackage();
        if (recvOtaAck(5000, MSG_TYPE_OTA_START, recvPayloadData))
        {
            break;
        }
    }

    if (retry < 0)
    {
        Serial.println("OTA开始失败");
        if (onErrorCallback)
            onErrorCallback(4); // 错误码4: OTA开始失败
        http.end();
        return false;
    }

    // 传输固件数据
    uint16_t packCurrent = 0;
    unsigned long totalTime = 0; // 总传输时间
    int packetCount = 0;         // 成功传输的包数量
    unsigned long progressStartTime = millis();
    unsigned long lastProgressUpdate = millis();

    // 继续传输剩余的固件数据
    while (packCurrent < pkgCount && http.connected())
    {
        // 定期显示进度
        if (millis() - progressStartTime > 1000)
        { // 每秒更新一次进度
            float progress = (float)packCurrent / pkgCount * 100;
            progressPercent = (int)progress;
            // Serial.printf("传输进度: %.1f%% (%d/%d)\r", progress, packCurrent, pkgCount);
            progressStartTime = millis();

            // 调用进度回调
            if (this->onProgressCallback)
            {
                this->onProgressCallback(progressPercent);
            }

            // 在适当的时间发送WebSocket进度
            if (this->webSocket && (millis() - lastProgressUpdate > 1500))
            {
                lastProgressUpdate = millis();
                JSONVar data_ota;
                data_ota["type"] = "voice_ota_progress";
                data_ota["data"] = progressPercent;
                data_ota["device_id"] = this->device_id;
                String sendData = JSON.stringify(data_ota);
                this->webSocket->sendTXT(sendData);

                // 调用LED动画回调
                if (this->ledAmi)
                {
                    this->ledAmi();
                }
            }
        }

        // 构造OTA数据包
        otaPack.msg_type = MSG_TYPE_OTA_FIRMWARE;
        memset(otaPayloadData, 0, OTA_PACK_LENGTH + 2);
        otaPayloadData[0] = packCurrent / 256;
        otaPayloadData[1] = packCurrent % 256;

        // 从流中读取数据到小缓冲区
        size_t bytesRead = 0;
        unsigned long readStartTime = millis();
        unsigned long lastDataTime = millis(); // 记录最后一次收到数据的时间
        const int MAX_RETRY_TIME = 3;          // 最大重试次数
        int retryCount = 0;

        while (bytesRead < OTA_PACK_LENGTH)
        {
            // 检查是否超时
            if (updateFailed())
            {
                Serial.println("升级超时，自动终止");
                if (onErrorCallback)
                    onErrorCallback(5); // 错误码5: 超时
                http.end();
                return false;
            }

            if (stream->available())
            {
                size_t currentRead = stream->readBytes(&smallBuffer[bytesRead], OTA_PACK_LENGTH - bytesRead);
                bytesRead += currentRead;
                lastDataTime = millis(); // 更新最后收到数据的时间
                retryCount = 0;          // 重置重试计数
                isUpdateProgress = true;

                // 最后一包数据可能不足OTA_PACK_LENGTH字节
                if (packCurrent == pkgCount - 1 && bytesRead > 0)
                {
                    break;
                }
            }

            // 超时检查 - 有数据传输时采用更短的超时，无数据时采用更长的超时
            if (bytesRead == 0)
            {
                // 尚未收到任何数据，使用更长的超时时间（30秒）
                if (millis() - readStartTime > 30000)
                {
                    Serial.println("读取固件数据超时（未收到任何数据）");
                    if (onErrorCallback)
                        onErrorCallback(3); // 错误码3: 固件下载失败
                    http.end();
                    return false;
                }
            }
            else
            {
                // 已收到部分数据，但超过5秒没有新数据
                if (millis() - lastDataTime > 5000)
                {
                    retryCount++;
                    if (retryCount > MAX_RETRY_TIME)
                    {
                        Serial.printf("读取固件数据超时（部分数据：%d/%d 字节）\r\n", bytesRead, OTA_PACK_LENGTH);
                        if (onErrorCallback)
                            onErrorCallback(3); // 错误码3: 固件下载失败
                        http.end();
                        return false;
                    }
                    Serial.printf("数据传输暂停，尝试继续接收（重试：%d/%d）\r\n", retryCount, MAX_RETRY_TIME);
                    lastDataTime = millis(); // 重置等待时间，给网络恢复的机会
                }
            }

            // 短暂延时以避免CPU占用过高
            delay(1);
        }

        // 更新包长度处理，支持部分数据读取
        if (bytesRead > 0)
        {
            // 将小缓冲区的数据复制到otaPayloadData
            memcpy(&otaPayloadData[2], smallBuffer, bytesRead);

            // 设置长度和CRC
            otaPack.len0 = (bytesRead + 6) / 256;
            otaPack.len1 = (bytesRead + 6) % 256;
            crc = crc16_ccitt(0, (uint8_t *)&otaPack, 5);
            crc = crc16_ccitt(crc, otaPayloadData, bytesRead + 2);
            otaPack.crc0 = crc / 256;
            otaPack.crc1 = crc % 256;
        }
        else
        {
            Serial.println("未能读取到任何数据");
            if (onErrorCallback)
                onErrorCallback(3); // 错误码3: 固件下载失败
            http.end();
            return false;
        }

        // 记录发送前的时间戳
        unsigned long packetStartTime = micros();

        // 发送数据包
        sendOtaPackage();

        // 接收ACK
        if (!recvOtaAck(5000, MSG_TYPE_OTA_FIRMWARE, recvPayloadData))
        {
            // 重试一次
            Serial.printf("第一次接收ACK超时，重试一次\r\n");
            sendOtaPackage();
            if (!recvOtaAck(3000, MSG_TYPE_OTA_FIRMWARE, recvPayloadData))
            {
                Serial.printf("OTA ack 返回超时\r\n");
                if (onErrorCallback)
                    onErrorCallback(6); // 错误码6: ACK接收超时
                http.end();
                return false;
            }
        }

        // 记录接收ACK后的时间戳并计算时间差
        unsigned long packetEndTime = micros();
        unsigned long transferTime = packetEndTime - packetStartTime;
        totalTime += transferTime;
        packetCount++;

        // 打印这包数据的传输时间
        if (packetCount % 20 == 0)
        { // 每20包打印一次传输时间，避免日志过多
            Serial.printf("包 %d 传输时间: %.3f ms\r\n", packCurrent, transferTime / 1000.0);
        }

        // 检查是否有错误
        if (recvPayloadData[0] == 1)
        {
            if ((recvPayloadData[1] != packCurrent / 256) || (recvPayloadData[2] != packCurrent % 256))
            {
                Serial.printf("OTA ack 返回的包序号不正确\r\n");
                if (onErrorCallback)
                    onErrorCallback(6); // 错误码6: ACK错误
                http.end();
                return false;
            }
        }
        else if (recvPayloadData[0] == 0)
        {
            // 错误处理
            int errorCode = 0;
            if ((int8_t)recvPayloadData[1] == -1)
            {
                Serial.printf("语音芯片端内存不足，不能ota\r\n");
                errorCode = 8; // 错误码8: 内存不足
            }
            else if ((int8_t)recvPayloadData[1] == -2)
            {
                Serial.printf("传输的固件不是ota打包的固件\r\n");
                errorCode = 9; // 错误码9: 固件格式错误
            }
            else if ((int8_t)recvPayloadData[1] == -3)
            {
                Serial.printf("传输的固件分区表信息校验错误\r\n");
                errorCode = 10; // 错误码10: 分区表错误
            }
            else if ((int8_t)recvPayloadData[1] == -4)
            {
                Serial.printf("传输的固件硬件版本号不一致\r\n");
                errorCode = 11; // 错误码11: 硬件版本不匹配
            }
            else
            {
                Serial.printf("OTA ACK返回其他错误 %d\r\n", (int8_t)recvPayloadData[1]);
                errorCode = 99; // 错误码99: 未知错误
            }
            if (onErrorCallback)
                onErrorCallback(errorCode);
            http.end();
            return false;
        }

        // 获取下一包序号
        packCurrent = recvPayloadData[3] * 256 + recvPayloadData[4];

        if (packCurrent >= pkgCount)
        { // 语音端确认收到了最后一包
            Serial.printf("last frame %d %d\r\n", packCurrent, pkgCount);

            // 打印平均传输时间
            if (packetCount > 0)
            {
                float avgTime = totalTime / (float)packetCount / 1000.0; // 转换为毫秒
                Serial.printf("总共传输 %d 包，平均每包传输时间: %.3f ms\r\n", packetCount, avgTime);
            }
            break;
        }
    }

    // 完成OTA
    bool success = false;
    if (otaFinish())
    {
        // 调用结束回调
        if (onEndCallback)
            onEndCallback();
        success = true;
    }
    else
    {
        if (onErrorCallback)
            onErrorCallback(4); // 错误码4: OTA完成失败
        success = false;
    }
    http.end();
    return success;
}

// 从URL下载固件并执行OTA升级
bool CIAS_ESP32_OTA::otaFromUrl(const char *firmwareUrl, const char *savePath)
{
    Serial.printf("开始从URL下载固件: %s\r\n", firmwareUrl);

    // 初始化WiFiClientSecure用于HTTPS请求
    WiFiClientSecure client;

    // 对于HTTPS，我们可能需要跳过证书验证（仅用于测试环境）
    client.setInsecure(); // 这会跳过SSL证书验证

    HTTPClient http;

    // 开始HTTPS请求
    if (!http.begin(client, firmwareUrl))
    {
        Serial.println("无法连接到服务器");
        return false;
    }

    // 添加必要的请求头
    http.addHeader("User-Agent", "ESP32-OTA-Updater");
    http.addHeader("Accept", "*/*");

    // 发送GET请求
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP请求失败，错误码: %d\r\n", httpCode);

        // 获取更多错误信息
        if (httpCode > 0)
        {
            String response = http.getString();
            Serial.printf("服务器响应: %s\r\n", response.c_str());
        }
        else
        {
            Serial.printf("连接错误: %s\r\n", http.errorToString(httpCode).c_str());
        }

        http.end();
        return false;
    }

    // 获取文件大小
    int contentLength = http.getSize();
    Serial.printf("固件大小: %d 字节\r\n", contentLength);

    // 检查FFat文件系统是否已挂载
    if (!FFat.begin())
    {
        Serial.println("FFat挂载失败");
        http.end();
        return false;
    }

    // 打开文件用于写入
    File file = FFat.open(savePath, "w");
    if (!file)
    {
        Serial.printf("无法创建文件: %s\r\n", savePath);
        http.end();
        return false;
    }

    // 获取HTTP响应流
    WiFiClient *stream = http.getStreamPtr();

    // 下载文件数据并写入文件
    size_t written = 0;
    uint8_t buffer[1024];
    unsigned long startTime = millis();

    while (http.connected() && (written < contentLength || contentLength == -1))
    {
        // 读取数据
        size_t size = stream->available();
        if (size > 0)
        {
            int bytesRead = stream->readBytes(buffer, min(size, sizeof(buffer)));
            if (bytesRead > 0)
            {
                file.write(buffer, bytesRead);
                written += bytesRead;

                // 打印下载进度
                if (contentLength > 0)
                {
                    float progress = (float)written / contentLength * 100;
                    Serial.printf("下载进度: %.1f%% (%d/%d)\r", progress, written, contentLength);
                }
            }
        }

        // 防止无限循环
        if (millis() - startTime > 600000)
        { // 10分钟超时
            Serial.println("下载超时");
            file.close();
            http.end();
            return false;
        }

        // 短暂延时以避免CPU占用过高
        delay(1);
    }

    Serial.println("\r\n下载完成");

    // 关闭文件和HTTP连接
    file.close();
    http.end();

    // 检查文件是否成功写入
    if (written == 0)
    {
        Serial.println("文件写入失败");
        return false;
    }

    Serial.printf("固件已保存至: %s\r\n", savePath);

    // 执行OTA升级
    Serial.println("开始OTA升级...");

    // 进入OTA模式
    if (!enterOtaMode())
    {
        Serial.println("进入OTA模式失败");
        return false;
    }

    // 延时确保进入OTA模式
    delay(1000);

    // 检查版本
    if (!otaCheckVersion())
    {
        Serial.println("版本检查失败");
        return false;
    }

    // 开始OTA传输
    if (!otaStart(savePath))
    {
        Serial.println("OTA开始失败");
        return false;
    }

    // 传输固件数据
    if (!otaTransportFirmware())
    {
        Serial.println("固件传输失败");
        return false;
    }

    // 完成OTA
    if (!otaFinish())
    {
        Serial.println("OTA完成失败");
        return false;
    }

    Serial.println("OTA升级成功");

    // 可选：删除下载的固件文件以释放空间
    // FFat.remove(savePath);

    return true;
}

/**
 * CI语音模块自动检测OTA升级函数
 * 修改说明：应用ota_manager中的升级逻辑，添加状态变量、进度检测和失败处理
 */
void ci_auto_update(
    const String &wake_word,
    const String &api_key,
    const String &domain,
    CIAS_ESP32_OTA &voiceChipOTA,
    ESP_AI &esp_ai,
    void (*awaitPlayerDone)(),                                       // 等待播放器完成的函数
    void (*playBuiltinAudio)(const unsigned char *data, size_t len), // 播放音频的函数
    void (*setChatMessage)(const String &text, const String &status))
{
    // Serial.println("系统检查CI语音模块升级中。");
    // vTaskDelay(1000 / portTICK_PERIOD_MS);
    // 添加升级状态变量，类似于ota_manager中的实现
    long startUpdateTime = 0;
    bool startUpdateEd = false;
    bool isUpdateProgress = false;
    bool willUpdate = false;

    HTTPClient http;
    String url = domain + "sdk/query_wake_ota";

    Serial.print("CI升级检查地址：");
    Serial.println(url);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // 构建JSON请求体
    JSONVar json_params;
    json_params["wake_word"] = wake_word;
    json_params["api_key"] = api_key;
    String send_data = JSON.stringify(json_params);

    // 发送POST请求
    int httpCode = http.POST(send_data);

    if (httpCode > 0)
    {
        String payload = http.getString();
        Serial.print("[HTTP] response: ");
        Serial.println(payload);
        Serial.printf("[HTTP] POST code: %d\n", httpCode);
        JSONVar parse_res = JSON.parse(payload);

        if (JSON.typeof(parse_res) == "undefined")
        {
            wait_mp3_player_done();
            playBuiltinAudio(huanx_gengxshibai_mp3, huanx_gengxshibai_mp3_len);
            wait_mp3_player_done();
            Serial.println("[Error] 响应解析失败");
            setChatMessage("唤醒词检测升级失败", "error");
        }
        else
        {
            // 检查是否有data字段
            if (parse_res.hasOwnProperty("data"))
            {
                JSONVar data = parse_res["data"];

                // 检查data对象中的latest字段
                if (data.hasOwnProperty("latest"))
                {
                    bool latest = (bool)data["latest"];

                    if (latest)
                    {
                        // setChatMessage("唤醒词已经是最新版本", "end");
                        setChatMessage("", "end");
                        Serial.println("唤醒词已经是最新版本，无需升级");
                    }
                    else
                    {

                        // 如果不是最新版本，检查data对象中的bin_url字段
                        if (data.hasOwnProperty("bin_url"))
                        {
                            String bin_url = (const char *)data["bin_url"];
                            if (bin_url != "" && bin_url != "null")
                            {
                                // 记录开始时间，类似于ota_manager中的实现
                                startUpdateTime = millis();
                                willUpdate = true;

                                if (!voiceChipOTA.begin())
                                {
                                    Serial.println("唤醒词初始化失败");
                                    setChatMessage("唤醒词初始化失败", "error");
                                    http.end();
                                    return;
                                }
                                bool otaResult; // 进入OTA模式
                                voiceChipOTA.enterOtaMode();
                                if (voiceChipOTA.otaCheckVersion())
                                {
                                    setChatMessage("正在更改唤醒词", "updating");
                                    Serial.println("检测到唤醒词新版本，开始升级...");
                                    // 执行OTA升级
                                    wait_mp3_player_done();
                                    playBuiltinAudio(huanx_jchegengx_mp3, huanx_jchegengx_mp3_len);
                                    wait_mp3_player_done();
                                    startUpdateEd = true;
                                    otaResult = voiceChipOTA.otaFromUrlStream(bin_url.c_str());
                                }
                                else
                                {
                                    Serial.println("天问模块固件不支持OTA升级/版本检测失败");
                                    http.end();
                                    return;
                                }
                                if (otaResult)
                                {
                                    // 检查是否有wake_word字段并保存
                                    if (data.hasOwnProperty("wake_word"))
                                    {
                                        String new_wake_word = (const char *)data["wake_word"];
                                        if (new_wake_word != "" && new_wake_word != "null")
                                        {
                                            // 调用esp_ai的setLocalData方法保存wake_word
                                            // 注意：这里需要esp_ai对象，可能需要通过参数传递或者使用全局对象
                                            esp_ai.setLocalData("wake_word", new_wake_word);
                                            Serial.print("保存新的唤醒词：");
                                            Serial.println(new_wake_word);
                                        }
                                    }
                                    ESP.restart(); // 升级成功后重启设备
                                }
                                else
                                {
                                    Serial.print("唤醒词升级失败, ");
                                    Serial.print("服务返回 code :");
                                    Serial.println(otaResult);

                                    // 添加失败检测逻辑，类似于ota_manager中的updateFailed方法
                                    if (voiceChipOTA.updateFailed())
                                    {
                                        Serial.println("唤醒词升级超时失败");
                                    }
                                    // wait_mp3_player_done();
                                    playBuiltinAudio(huanx_gengxshibai_mp3, huanx_gengxshibai_mp3_len);
                                    // wait_mp3_player_done();
                                    setChatMessage("唤醒词升级失败", "error");
                                    Serial.println("唤醒词升级失败！");
                                    wait_mp3_player_done();
                                    // 升级失败后重启设备
                                    // 更新失败后需要让设备还能继续使用,不要重启设备
                                    // ESP.restart();
                                }
                                willUpdate = false;
                                voiceChipOTA.end();
                            }
                            else
                            {
                                wait_mp3_player_done();
                                playBuiltinAudio(huanx_gengxshibai_mp3, huanx_gengxshibai_mp3_len);
                                wait_mp3_player_done();
                                setChatMessage("唤醒词升级包URL无效", "end");
                                Serial.println("唤醒词升级包URL无效");
                                ESP.restart(); // 升级失败后重启设备
                            }
                        }
                        else
                        {
                            wait_mp3_player_done();
                            playBuiltinAudio(huanx_gengxshibai_mp3, huanx_gengxshibai_mp3_len);
                            wait_mp3_player_done();
                            setChatMessage("唤醒词不需要升级", "end");
                            Serial.println("唤醒词不需要升级或缺少升级包URL");
                            ESP.restart(); // 升级失败后重启设备
                        }
                    }
                }
                else
                {
                    wait_mp3_player_done();
                    playBuiltinAudio(huanx_gengxshibai_mp3, huanx_gengxshibai_mp3_len);
                    wait_mp3_player_done();
                    setChatMessage("唤醒词检测升级失败", "error");
                    Serial.println("响应数据中缺少latest字段");
                    ESP.restart(); // 升级失败后重启设备
                }
            }
            else
            {
                wait_mp3_player_done();
                playBuiltinAudio(huanx_gengxshibai_mp3, huanx_gengxshibai_mp3_len);
                wait_mp3_player_done();
                setChatMessage("唤醒词检测升级失败", "error");
                Serial.println("响应数据中缺少data字段");
                ESP.restart(); // 升级失败后重启设备
            }
        }
    }
    else
    {
        wait_mp3_player_done();
        playBuiltinAudio(huanx_gengxshibai_mp3, huanx_gengxshibai_mp3_len);
        wait_mp3_player_done();
        setChatMessage("唤醒词检测升级失败", "error");
        Serial.printf("[Error HTTP] 唤醒词检测升级失败: %s", url.c_str());
        ESP.restart(); // 升级失败后重启设备
    }
    http.end();
};