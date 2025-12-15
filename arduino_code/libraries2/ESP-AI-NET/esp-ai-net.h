/*
 * MIT License
 *
 * Copyright (c) 2025-至今 小明IO
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author 小明IO
 * @email  1746809408@qq.com
 * @github https://github.com/wangzongming/esp-ai
 * @websit https://espai.fun
 */

#include <Arduino.h>
#include "HardwareSerial.h"
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
#include <vector>
#include <functional>
#include <stdlib.h>
#include <time.h>

#pragma once

// 调试开关
#define IS_DEBUG

#ifdef IS_DEBUG
#define LOG_I(fmt, ...) printf_P(("[ESP-AI-NET][%s][%d]:" fmt "\r\n"), __func__, __LINE__, ##__VA_ARGS__)
#define LOG_E(fmt, ...) printf_P(("[ESP-AI-NET][%s][%s][%d]:" fmt "\r\n"), __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...)
#endif

/**
 * 4G WebSocket状态机
 */
enum EAN_WSConnectState
{
    EAN_WS_DISCONNECTED,
    EAN_WS_CONNECTING,
    EAN_WS_HANDSHAKE,
    EAN_WS_CONNECTED
};

/**
 * 数据发送模式
 */
enum EAN_TCPDataModel
{
    EAN_TCP_MODEL_TRANSPARENT,
    EAN_TCP_MODEL_DATA
};

/**
 * WebSocket操作码枚举
 */
typedef enum
{
    EAN_WSop_continuation = 0x00, // 继续帧
    EAN_WSop_text = 0x01,         // 文本帧
    EAN_WSop_binary = 0x02,       // 二进制帧
    EAN_WSop_close = 0x08,        // 关闭连接
    EAN_WSop_ping = 0x09,         // Ping
    EAN_WSop_pong = 0x0A          // Pong
} EAN_WSopcode_t;

#define SERIAL_DATA_TASK_SIZE 1024 * 5
// 串口发送数据分片大小
#define WS_CHUNK_SIZE 1024 * 4

typedef enum
{
    EAN_WStype_ERROR,
    EAN_WStype_DISCONNECTED,
    EAN_WStype_CONNECTED,
    EAN_WStype_TEXT,
    EAN_WStype_BIN,
    EAN_WStype_FRAGMENT_TEXT_START,
    EAN_WStype_FRAGMENT_BIN_START,
    EAN_WStype_FRAGMENT,
    EAN_WStype_FRAGMENT_FIN,
    EAN_WStype_PING,
    EAN_WStype_PONG,
} EAN_WStype_t;

typedef std::function<void(EAN_WStype_t type, uint8_t *payload, size_t length)> WebSocketClientEvent;

struct SerialDataTaskContext
{
    HardwareSerial *serial_4g;
    SemaphoreHandle_t serialDataMutex;
    String *serialData;
    int *TCPConnectId;

    EAN_TCPDataModel (*getTCPDataModel)();
    // void (*wsOnData)(EAN_WStype_t type, uint8_t *payload, size_t length);
    // void (*wsOnData)(uint8_t *payload, size_t length);
    // void (*feedWsStream)(const uint8_t *data, size_t len);
    std::function<void(const uint8_t *data, size_t len)> feedWsStream;
    // bool (*sendBIN)(const uint8_t *data, size_t len, uint8_t opcodeFirst);
};

class ESP_AI_NET
{
public:
    ESP_AI_NET();
    static ESP_AI_NET *instance; // 静态实例指针

    void begin();

    /**
     * get 请求
     * @param host 主机地址, 例如: api.espai.fun
     * @param path 请求路径, 例如: /sdk/use_once_api_by_api_key?api_key=xxx
     *
     *
     * 涉及下面 AT 命令
     * AT+MHTTPTERM=0
     * AT+MHTTPDEL=0
     * AT+MHTTPCREATE="http://api.espai.fun"
     * AT+MHTTPCREATE="http://120.237.117.213:9997/"
     * AT+MHTTPREQUEST=0,1,0,"/sdk/use_once_api_by_api_key?api_key=xxx"
     */
    String GET(const String &host, const String &path);

    /**
     * post 请求
     * 会自动增加头信息 "Content-Type: application/json"
     *
     * @param host 主机地址, 例如: api.espai.fun
     * @param path 请求路径, 例如: /devices/add
     * @param params 请求参数,需要进行 stringify 后传入, 例如: { "name": "张三" }
     *
     *
     * 涉及下面 AT 命令
     * AT+MHTTPTERM=0
     * AT+MHTTPDEL=0
     * AT+MHTTPCREATE="http://api.espai.fun"
     * AT+MHTTPHEADER=0,0,0,"Content-Type: application/json"
     * AT+MHTTPCONTENT=0,0,0,"{"api_key":"1227f3e384d04369b350c04429b36b88","device_id":"xxx","bin_id":"111","wifi_ssid":"xxx", "version": "0.1"}"
     * AT+MHTTPREQUEST=0,2,0,"/devices/add"
     *
     */
    String POST(const String &host, const String &path, const String &params);

    // 发起WS连接
    bool connectWS(const String &host, const int &port, const String &path);

    /**
     *  向 WS 服务发送文本
     *  bool ok = sendTXT("...很长很长的文本...");
     *  if (!ok) {
     *     Serial.println("发送失败");
     *  }
     */
    bool sendTXT(const String &txt, uint8_t opcodeFirst = EAN_WSop_text);

    /**
     * 向 WS 服务发送二进制文件
     *
     * 主函数：把 data(len) 按 WS_CHUNK_SIZE 拆包并发送（文本模式使用 opcode = 0x1）
     * 返回 true 表示成功发送所有片段，false 表示未发送（例如互斥超时/串口失败）
     *
     * sendBIN(bin.data(), bin.size(), 0x2);
     */
    bool sendBIN(const uint8_t *data, size_t len, uint8_t opcodeFirst = EAN_WSop_binary);

    /**
     * 消息监听
     */
    void onEvent(WebSocketClientEvent cbEvent);


    // 友元声明（允许访问 private 成员）
    friend EAN_TCPDataModel getTCPDataModel_impl();
    // friend void wsOnData_impl(EAN_WStype_t type, uint8_t *payload, size_t length);

private:
    // 网络是否准备成功
    bool networkReady = false;

    StaticTask_t serialDataTaskBuffer;
    StackType_t serialDataTaskStack[SERIAL_DATA_TASK_SIZE];
    SerialDataTaskContext serialDataTask_ctx;
    // 当前串口字符串数据
    String serialData = "";
    SemaphoreHandle_t serialDataMutex = xSemaphoreCreateMutex();
    // 是否正在请求执行AT命令
    SemaphoreHandle_t atExecMutex = xSemaphoreCreateMutex();
    // 最多等待串口返回时间，单位 ms
    int serialMaxAwaitTime = 0;

    /**
     * 4G模块通信串口设置
     */
    HardwareSerial *serial_4g;

    /**
     * 初始化串口
     */
    void initSerial4G(int uart_num, int baudrate, int rx_pin, int tx_pin);

    /**
     * 初始化监听串口任务
     */
    void monitorSerial();

    /**
     * 接口参数提取方法
     */
    String mergeJsonFromHTTP(const String &raw);

    /**
     * 生成WebSocket密钥
     * 作用：生成握手所需的随机密钥
     * 必要性：WebSocket握手协议要求
     */
    String generateWebSocketKey();

    /**
     * Base64编码
     * 作用：将二进制数据编码为Base64字符串
     * 必要性：WebSocket握手和认证需要Base64编码
     */
    String base64Encode(const uint8_t *data, size_t length);

    /**
     * 判断返回的结果是否成功
     */
    bool isResponseOK(const String &response, const String &endStr = "OK\n");

    /**
     * 发送AT命令
     * 结束标识符是什么
     */
    String sendATCommand(const String &command, int awaitTime = 300, const String &endStr = "OK");

    /**
     * 使用的 TCP 通道, 999 代表无连接
     */
    int TCPConnectId = 999;

    /**
     * WS连接状态
     */
    EAN_WSConnectState connectState = EAN_WS_DISCONNECTED;
    SemaphoreHandle_t connectStateMutex = xSemaphoreCreateMutex();
    void setWSState(EAN_WSConnectState state)
    {
        if (xSemaphoreTake(connectStateMutex, pdMS_TO_TICKS(portMAX_DELAY)) == pdTRUE)
        {
            connectState = state;
            xSemaphoreGive(connectStateMutex);
        }
    }
    EAN_WSConnectState getWSState()
    {
        return connectState;
    }

    /**
     * TCP 数据发送模式
     */
    EAN_TCPDataModel tcpDataModel = EAN_TCP_MODEL_DATA;
    SemaphoreHandle_t tcpDataModelMutex = xSemaphoreCreateMutex();
    bool setTCPDataModel(EAN_TCPDataModel state)
    {
        if (xSemaphoreTake(tcpDataModelMutex, pdMS_TO_TICKS(10000)) == pdTRUE)
        {

            // 从透传模式切到普通模式
            if (state == EAN_TCP_MODEL_DATA)
            {
                if (getTCPDataModel() == EAN_TCP_MODEL_TRANSPARENT)
                {
                    // 退出透传
                    // ML307R 在退出透传的 “+++” 检测机制中有一个静默窗口（一般是 1 秒内无数据）。
                    // 如果你在发完数据后立即发送 "+++"，模块可能误判数据流中有噪音，导致连接异常。
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    serial_4g->print("+++");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                LOG_I("已切换为普通模式");
            }

            // 切换为透传模式
            if (state == EAN_TCP_MODEL_TRANSPARENT)
            {
                while (getTCPDataModel() == EAN_TCP_MODEL_TRANSPARENT)
                {
                    LOG_I("等待其他任务透传完毕...");
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                String resMode = sendATCommand("AT+MIPMODE=" + String(TCPConnectId) + ",1", 3000, "CONNECT");
                if (!isResponseOK(resMode, "CONNECT"))
                {
                    LOG_E("进入透传模式失败: %s", resMode.c_str());
                    return false;
                }
                LOG_I("已切换为透传模式");
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            tcpDataModel = state;
            xSemaphoreGive(tcpDataModelMutex);
            return true;
        }
        else
        {
            LOG_I("设置数据模式失败, 等待 tcpDataModelMutex 锁失败.");
            return false;
        }
    }

    EAN_TCPDataModel getTCPDataModel()
    {
        return tcpDataModel;
    }

    // 内部：构造单个 WebSocket 帧（会把 payload 做 mask 后加入到返回的 vector）
    // opcode: 0x1=text, 0x2=binary, 0x0=continuation
    static std::vector<uint8_t> build_ws_frame_chunk(const uint8_t *payload, size_t payload_len, uint8_t opcode, bool fin)
    {
        std::vector<uint8_t> frame;
        frame.reserve(2 + 9 + 4 + payload_len); // 预留，减少 realloc

        // 1. FIN + opcode
        uint8_t b0 = (fin ? 0x80 : 0x00) | (opcode & 0x0F);
        frame.push_back(b0);

        // 2. Mask bit = 1 (client->server must be masked)
        if (payload_len < 126)
        {
            frame.push_back(static_cast<uint8_t>(0x80 | payload_len));
        }
        else if (payload_len < 65536)
        {
            frame.push_back(static_cast<uint8_t>(0x80 | 126));
            frame.push_back(static_cast<uint8_t>((payload_len >> 8) & 0xFF));
            frame.push_back(static_cast<uint8_t>((payload_len >> 0) & 0xFF));
        }
        else
        {
            frame.push_back(static_cast<uint8_t>(0x80 | 127));
            // 8 bytes big-endian length
            for (int i = 7; i >= 0; --i)
            {
                frame.push_back(static_cast<uint8_t>((payload_len >> (8 * i)) & 0xFF));
            }
        }

        // 3. 随机掩码（请在 setup() 中调用 srand(seed) 初始化一次）
        uint8_t mask[4];
        for (int i = 0; i < 4; ++i)
            mask[i] = static_cast<uint8_t>(rand() & 0xFF);
        frame.insert(frame.end(), mask, mask + 4);

        // 4. Masked payload
        for (size_t i = 0; i < payload_len; ++i)
        {
            uint8_t m = payload[i] ^ mask[i % 4];
            frame.push_back(m);
        }

        return frame;
    }

    /**
     * 获取可用的 tcp 通道
     */
    static int getFirstInitialChannel(const String &response);

    /**
     * ws 收到数据后的回调
     */
    // void wsOnData(EAN_WStype_t type, uint8_t *payload, size_t length);

    /**
     * 给用户的回调
     */
    // WebSocketClientEvent wsOnDataCb = nullptr;
    void feedWsStream(const uint8_t *data, size_t len);
};

