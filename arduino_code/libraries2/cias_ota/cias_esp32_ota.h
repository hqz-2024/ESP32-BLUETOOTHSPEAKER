#ifndef CIAS_ESP32_OTA_H
#define CIAS_ESP32_OTA_H

// 在文件开头的包含部分添加
#include <Arduino.h>
#include <HardwareSerial.h>
#include <FFat.h>       // 使用FFat库来访问ESP32内部分区
#include <HTTPClient.h> // 添加HTTPClient头文件用于HTTP下载
// #include <Arduino_JSON.h> // 添加JSON库支持
#include <esp-ai.h>
// 修改引用路径，使用库内部的audio目录
#if defined(ESP_AI_LANGUAGE_EN)
#include "audio/en/huanx_gengxshibai_mp3.h"
#include "audio/en/huanx_jchegengx_mp3.h"
#elif defined(ESP_AI_LANGUAGE_JA)
//
//
#elif defined(ESP_AI_LANGUAGE_ZH)
#include "audio/zh/huanx_gengxshibai.h"
#include "audio/zh/huanx_jchegengx.h"
#endif

// 添加对cias_crc.h的包含，以正确引用CRC16函数
#ifdef __cplusplus
extern "C"
{
#endif
#include "cias_crc.h"
#ifdef __cplusplus
}
#endif

// 包含原始配置文件以保持兼容性
#include "cias_ota_config.h"

// 添加UART接收缓冲区大小定义
#define UART_RCV_BUF_SIZE 20

// 添加uart_msg_t结构体定义
typedef struct
{
    long mtype;                       // 消息类型
    uint8_t mtext[UART_RCV_BUF_SIZE]; // 消息文本
} uart_msg_t;

class CIAS_ESP32_OTA
{
private:
    const String &device_id;
    HardwareSerial *serialPort;
    QueueHandle_t uartQueue;
    TaskHandle_t uartReadTaskHandle;
    File otaFile;
    uint32_t fileSize;
    uint32_t pkgCount;
    cias_ota_pack_t otaPack;
    uint8_t otaPayloadData[OTA_PACK_LENGTH + 2];
    uint8_t otaUartSendData[OTA_PACK_LENGTH + 8];

    // 添加OTA状态变量
    long startUpdateTime;
    bool startUpdateEd;
    bool isUpdateProgress;
    bool willUpdate;
    int progressPercent;

    // 添加回调函数指针
    static void (*onStartCallback)(void);
    static void (*onEndCallback)(void);
    static void (*onProgressCallback)(int percent);
    static void (*onErrorCallback)(int errorCode);

    // 添加从ota_manager参考的回调函数指针
    WebSocketsClient *webSocket;
    void (*showNotification)(const char *data);
    void (*onlyShowNotification)(bool only_show_notification);
    // void (*onProgress)(int percent); // 移到public部分
    void (*stopSession)(void);
    void (*delAllTask)(void);
    void (*awaitPlayerDone)(void);
    void (*playBuiltinAudio)(const unsigned char *data, size_t len);
    void (*ledAmi)(void);

    // 静态成员变量，用于存储this指针
    static CIAS_ESP32_OTA *instance;

    // CRC16计算
    uint16_t crc16_ccitt(uint16_t preCrc, const uint8_t *data, uint32_t length);
    uint16_t getOtaPackPayloadLen(cias_ota_pack_t *otaPack);
    void ciasOtaDataInit(void);
    void ciasOtaDataCheckVersion();
    bool ciasOtaDataStart(const char *otaFileName);
    bool ciasOtaDataFirmware(uint16_t packCount);
    void ciasOtaDataFinish();
    int sendOtaPackage();
    bool verifyOtaAck(uint8_t *data, const uint8_t recvMsgType);
    bool recvOtaAck(int timeoutMs, const uint8_t recvMsgType, uint8_t *recvPayloadData);
    bool recvEnterOtaModeAck(int timeoutMs);
    static void uartReadTask(void *pvParameters);

public:
    // 将onProgress移到public部分
    void (*onProgress)(int percent);

    // 修改构造函数，添加与ota_manager相同的参数
    CIAS_ESP32_OTA(
                    const String &deviceId,
                    HardwareSerial *serial,
                    WebSocketsClient *webSocket,
                    void (*ShowNotification)(const char *data),
                    void (*OnlyShowNotification)(bool only_show_notification),
                    void (*onProgress)(int percent),
                    void (*stopSessionCb)(),
                    void (*delAllTaskCb)(),
                    void (*awaitPlayerDoneCb)(),
                    void (*playBuiltinAudioCb)(const unsigned char *data, size_t len),
                    void (*ledAmiCb)());
    ~CIAS_ESP32_OTA();
    bool begin();
    void end();
    bool enterOtaMode();
    bool otaCheckVersion();
    bool otaStart(const char *otaFileName);
    bool otaTransportFirmware();
    bool otaFinish();
    // 添加新方法：从URL下载固件并执行OTA
    bool otaFromUrl(const char *firmwareUrl, const char *savePath = "/firmware.bin");
    bool otaFromUrlStream(const char *firmwareUrl);

    // 添加reset方法，用于重置对象状态
    void reset();

    // // 添加设置回调函数的方法
    // void onStart(void (*callback)(void));
    // void onEnd(void (*callback)(void));
    // void onProgress(void (*callback)(int percent));
    // void onError(void (*callback)(int errorCode));

    // 添加获取OTA状态的方法
    bool isUpdating() const;
    int getProgress() const;
    bool updateFailed() const;
};

// 全局函数声明
void ci_auto_update(
    const String &wake_word,
    const String &api_key,
    const String &domain,
    CIAS_ESP32_OTA &voiceChipOTA,
    ESP_AI &esp_ai,
    void (*awaitPlayerDone)(), // 等待播放器完成的函数
    void (*playBuiltinAudio)(const unsigned char *data, size_t len), // 播放音频的函数
    void (*setChatMessage)(const String &text, const String &status));

#endif // CIAS_ESP32_OTA_H

// 在文件开头添加错误码定义
// 添加OTA错误码定义
enum OTA_ERROR_CODE
{
    OTA_ERROR_NONE = 0,
    OTA_ERROR_ENTER_MODE = 1,
    OTA_ERROR_HTTP_CONNECT = 2,
    OTA_ERROR_FIRMWARE_DOWNLOAD = 3,
    OTA_ERROR_OTA_FINISH = 4,
    OTA_ERROR_TIMEOUT = 5,
    OTA_ERROR_ACK_TIMEOUT = 6,
    OTA_ERROR_VERSION = 7,
    OTA_ERROR_MEMORY = 8,
    OTA_ERROR_FIRMWARE_FORMAT = 9,
    OTA_ERROR_PARTITION = 10,
    OTA_ERROR_HARDWARE_VERSION = 11,
    OTA_ERROR_UNKNOWN = 99
};