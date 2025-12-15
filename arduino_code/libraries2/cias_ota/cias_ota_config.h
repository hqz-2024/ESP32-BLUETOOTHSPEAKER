#ifndef __CIAS_OTA_CONFIG_H__
#define __CIAS_OTA_CONFIG_H__
#include <stdint.h>
#include <stdbool.h>
#define DEBUG_LOG 1
#define OUT_DEBUG_INFO 1

#define OTA_INVALID_HEAD_LENGTH            0x8000                 //ota时只需要从0x8000开始取数据，前面是boot等无效信息
#define OTA_UART_BAUDRATE                  921600                 // 最大3000000
#define OTA_PACK_LENGTH                    1024                   //每包有效数据长度

#define MSG_HEAD_HIGH                      0xA5
#define MSG_HEAD_LOW                       0x0F
#define MSG_TAIL                           0xFF
#define MSG_TYPE_OTA_VERSION 	           0xA0
#define MSG_TYPE_OTA_START 	               0xA1
#define MSG_TYPE_OTA_FIRMWARE 	           0xA2
#define MSG_TYPE_OTA_FINISH 	           0xA3
#define MSG_TYPE_OTA_REQUEST 	           0xA4
// 传输包解析结构类型，用于构造和解包
#pragma pack(1)
typedef struct
{
	unsigned char head0;
	unsigned char head1;
	unsigned char len0;
	unsigned char len1;
	unsigned char msg_type;
	unsigned char *data;
	unsigned char crc0;
	unsigned char crc1;
	unsigned char tail;
}cias_ota_pack_t;
#pragma pack()

bool ota_check_version();
bool ota_start(char *ota_file_name);
bool ota_transport_firmware();
bool ota_finish();
int send_ota_package();
bool recv_ota_ack(int timeout_ms, const uint8_t recv_msg_type, uint8_t *recv_payload_data);
#endif


