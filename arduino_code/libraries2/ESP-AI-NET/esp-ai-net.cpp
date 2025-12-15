/**
 * Copyright (c) 2024 小明IO
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Commercial use of this software requires prior written authorization from the Licensor.
 * 请注意：将 ESP-AI 代码用于商业用途需要事先获得许可方的授权。
 * 删除与修改版权属于侵权行为，请尊重作者版权，避免产生不必要的纠纷。
 *
 * @author 小明IO
 * @email  1746809408@qq.com
 * @github https://github.com/wangzongming/esp-ai
 * @websit https://espai.fun
 */

#include "esp-ai-net.h"

// 全局定义（或静态定义）
ESP_AI_NET *ESP_AI_NET::instance = nullptr;

ESP_AI_NET::ESP_AI_NET()
{
}

void ESP_AI_NET::begin()
{
	// 例如用芯片 ID 或系统时间来做 seed 更随机
	srand((unsigned)millis());

	// 初始化串口, UART1, RX=45, TX=48
	initSerial4G(1, 115200, 45, 48);

	// 监听 4g 串口数据
	monitorSerial();

	String res = sendATCommand("AT", 1000);
	LOG_I("AT 响应 <==: %s", res.c_str());
	while (!isResponseOK(res))
	{
		LOG_E("4G 模块无法初始化, AT 指令响应失败: %s", res.c_str());
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		res = sendATCommand("AT", 1000);
	}

	String resCSQ = sendATCommand("AT+CSQ", 1000);
	LOG_I("AT+CSQ 响应 <==: %s", resCSQ.c_str());
	while (!isResponseOK(resCSQ))
	{
		LOG_E("4G 模块无法初始化, AT+CSQ 信号查询指令响应失败: %s", resCSQ.c_str());
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		resCSQ = sendATCommand("AT+CSQ", 1000);
	}

	while (resCSQ.indexOf("+CSQ: 99,") != -1)
	{
		LOG_E("4G 模块信号异常,请检查SIM卡或天线");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		resCSQ = sendATCommand("AT+CSQ", 1000);
	}

	String resCGATT = sendATCommand("AT+CGATT?", 1000);
	LOG_I("AT+CGATT? 响应 <==: %s", resCGATT.c_str());
	while (!isResponseOK(resCGATT))
	{
		LOG_E("4G 模块无法初始化, AT+CGATT? PDP 激活响应失败: %s", resCGATT.c_str());
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		resCGATT = sendATCommand("AT+CGATT?", 1000);
	}
	networkReady = true;
}

void ESP_AI_NET::initSerial4G(int uart_num, int baudrate, int rx_pin, int tx_pin)
{
	LOG_I("初始化串口, 串口:%d, 波特率:%d, rx_pin:%d, tx_pin:%d", uart_num, baudrate, rx_pin, tx_pin);
	serial_4g = new HardwareSerial(uart_num);
	serial_4g->begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);
}

bool ESP_AI_NET::isResponseOK(const String &response, const String &endStr)
{
	if (response.indexOf(endStr) != -1)
	{
		return true;
	}
	return false;
}

std::vector<String> splitString(const String &input, char delimiter = ',')
{
	std::vector<String> result;
	int start = 0;
	int index = input.indexOf(delimiter);

	while (index >= 0)
	{
		String token = input.substring(start, index);
		token.trim();
		result.push_back(token);
		start = index + 1;
		index = input.indexOf(delimiter, start);
	}

	// 最后一个
	String last = input.substring(start);
	last.trim();
	result.push_back(last);

	return result;
}

// 回调类型
typedef void (*wsCallback_t)(EAN_WStype_t type, uint8_t *payload, size_t length);

// WebSocket 解码函数
std::vector<uint8_t> wsDecode(const uint8_t *data, size_t length)
{
	// if (length < 2)
	// 	return {};

	size_t pos = 0;
	uint8_t byte1 = data[pos++];
	uint8_t byte2 = data[pos++];

	bool fin = byte1 & 0x80;
	uint8_t opcode = byte1 & 0x0F;

	bool masked = byte2 & 0x80;
	uint64_t payloadLen = byte2 & 0x7F;

	if (payloadLen == 126)
	{
		// if (length < pos + 2)
		// 	return;
		payloadLen = (data[pos] << 8) | data[pos + 1];
		pos += 2;
	}
	else if (payloadLen == 127)
	{
		// if (length < pos + 8)
		// 	return;
		payloadLen = 0;
		for (int i = 0; i < 8; i++)
		{
			payloadLen = (payloadLen << 8) | data[pos + i];
		}
		pos += 8;
	}

	uint8_t mask[4] = {0};
	if (masked)
	{
		// if (length < pos + 4)
		// 	return;
		memcpy(mask, &data[pos], 4);
		pos += 4;
	}

	// if (length < pos + payloadLen)
	// 	return;

	std::vector<uint8_t> payload(data + pos, data + pos + payloadLen);

	// 客户端发送的数据有掩码
	if (masked)
	{
		for (size_t i = 0; i < payload.size(); i++)
		{
			payload[i] ^= mask[i % 4];
		}
	}

	// // 调用回调
	// if (cb)
	// {
	// 	EAN_WStype_t type = (opcode == 0x1) ? EAN_WStype_TEXT : EAN_WStype_BIN;
	// 	cb(type, payload.data(), payload.size());
	// }
	return payload;
}

// 示例回调
void myWsCallback(EAN_WStype_t type, uint8_t *payload, size_t length)
{
	Serial.print("[WS] 类型: ");
	Serial.print(type == EAN_WStype_TEXT ? "TEXT" : "BIN");
	Serial.print(", 内容: ");
	for (size_t i = 0; i < length; i++)
	{
		Serial.print((char)payload[i]);
	}
	Serial.println();
}

// 字符串转二进制
std::vector<uint8_t> stringToBinary(const String &str)
{
	std::vector<uint8_t> buf;

	// 判断是否是十六进制字符串（长度为偶数，且全部是 0-9A-F）
	bool isHex = str.length() % 2 == 0;
	for (size_t i = 0; i < str.length() && isHex; i++)
	{
		char c = str.charAt(i);
		if (!isxdigit(c))
		{
			isHex = false;
		}
	}

	if (isHex)
	{
		// 十六进制字符串转二进制
		for (size_t i = 0; i < str.length(); i += 2)
		{
			String byteStr = str.substring(i, i + 2);
			uint8_t byte = (uint8_t)strtoul(byteStr.c_str(), NULL, 16);
			buf.push_back(byte);
		}
	}
	else
	{
		// 普通文本直接转 uint8_t
		for (size_t i = 0; i < str.length(); i++)
		{
			buf.push_back((uint8_t)str.charAt(i));
		}
	}

	return buf;
}

// void serialDataTask(void *arg)
// {
// 	SerialDataTaskContext *ctx = static_cast<SerialDataTaskContext *>(arg);
// 	if (ctx == nullptr)
// 	{
// 		LOG_E("[Error] serialDataTask ctx is null!");
// 		vTaskDelete(NULL);
// 		return;
// 	}

// 	LOG_I("开始监听串口数据");

// 	String lineBuffer = "";
// 	while (1)
// 	{
// 		while (ctx->serial_4g->available())
// 		{
// 			char c = ctx->serial_4g->read();
// 			// Serial.println(c);

// 			if (c == '\r' || c == '\n')
// 			{
// 				if (lineBuffer.length() > 0)
// 				{
// 					// 完整的一行响应
// 					xSemaphoreTake(ctx->serialDataMutex, portMAX_DELAY);
// 					lineBuffer.trim();
// 					*(ctx->serialData) += lineBuffer + "\n";
// 					xSemaphoreGive(ctx->serialDataMutex);
// 					// LOG_I("---%s", lineBuffer.c_str());

// 					if (lineBuffer.startsWith("+MIPURC: \"rtcp\""))
// 					{
// 						// static const String lb = lineBuffer;
// 						// LOG_I("---%s", lineBuffer.c_str());
// 						// LOG_I("---%s", lb.c_str());

// 						// static std::vector<String> parts = splitString(lb, ',');
// 						// if((int )parts[1] == TCPConnectId){ }

// 						// Serial.print("通道ID: ");
// 						// Serial.print(parts[1]);
// 						// Serial.print(" 长度: ");
// 						// Serial.print(parts[2]);
// 						// Serial.print(" 数据: ");
// 						// static auto dataBin = stringToBinary(parts[3]);

// 						// // String playload = parts[3];
// 						// // Serial.println();
// 						// // auto data = stringToBinary(playload);
// 						// Serial.print("\n原始数据:");
// 						// for (auto b : dataBin)
// 						// {
// 						// 	Serial.print(b, HEX);
// 						// 	Serial.print(" ");
// 						// }
// 						// Serial.println();

// 						// Serial.print(" 数据(hex): ");
// 						// String dataStr = parts[3];
// 						// for (size_t i = 0; i < dataStr.length(); i++)
// 						// {
// 						// 	uint8_t byte = (uint8_t)dataStr[i];
// 						// 	if (byte < 16)
// 						// 		Serial.print('0');
// 						// 	Serial.print(byte, HEX);
// 						// 	Serial.print(' ');
// 						// }
// 						// Serial.println();

// 						// String playload = parts[3];
// 						// Serial.println();
// 						// auto data = stringToBinary(playload);
// 						// Serial.print("原始数据:");
// 						// for (auto b : data)
// 						// {
// 						// 	Serial.print(b, HEX);
// 						// 	Serial.print(" ");
// 						// }
// 						// Serial.println();

// 						// Serial.print("转吗数据:");
// 						// for (auto b : wsDecode(data.data(), sizeof(data.data())))
// 						// {
// 						// 	Serial.print(b, HEX);
// 						// 	Serial.print(" ");
// 						// }
// 						// Serial.println();

// 						// wsDecode(parts[3], sizeof(testFrame), myWsCallback);

// 						// // 16进制输出
// 						// for (auto b : parts[3])
// 						// {
// 						// 	if (b < 16)
// 						// 		Serial.print('0');
// 						// 	Serial.print(b, HEX);
// 						// 	Serial.print(' ');
// 						// }
// 						// Serial.println();
// 					}

// 					// // 16进制输出
// 					// for (auto b : lineBuffer)
// 					// {
// 					// 	if (b < 16)
// 					// 		Serial.print('0');
// 					// 	Serial.print(b, HEX);
// 					// 	Serial.print(' ');
// 					// }
// 					// Serial.println();
// 					lineBuffer = "";
// 				}
// 			}
// 			else
// 			{
// 				lineBuffer += c;
// 			}
// 		}

// 		// 如果长时间未有换行符，也可能是被拆包的情况
// 		if (lineBuffer.length() > 1024)
// 		{
// 			xSemaphoreTake(ctx->serialDataMutex, portMAX_DELAY);
// 			lineBuffer.trim();
// 			*(ctx->serialData) += lineBuffer + "\n";
// 			xSemaphoreGive(ctx->serialDataMutex);

// 			LOG_I("---- %s", lineBuffer.c_str());
// 			// // 16进制输出
// 			// for (auto b : lineBuffer)
// 			// {
// 			// 	if (b < 16)
// 			// 		Serial.print('0');
// 			// 	Serial.print(b, HEX);
// 			// 	Serial.print(' ');
// 			// }
// 			// Serial.println();

// 			lineBuffer = "";
// 		}

// 		vTaskDelay(10 / portTICK_PERIOD_MS);
// 	}
// }

// void wsOnData(uint8_t *data, int len)
// {
// 	if (len < 2)
// 		return;
// 	uint8_t opcode = data[0] & 0x0F;
// 	uint8_t payloadLen = data[1] & 0x7F;
// 	int offset = 2;

// 	if (opcode == 0x1)
// 	{
// 		LOG_I("[WS] 文本帧: %.*s", payloadLen, data + offset);
// 	}
// 	else if (opcode == 0x2)
// 	{
// 		LOG_I("[WS] 二进制帧长度: %d", payloadLen);
// 	}
// }

// 流缓冲区（保留未解析字节）
static std::vector<uint8_t> wsStreamBuffer;

// 将接收到的原始字节追加并尽可能解析完整的 WebSocket 帧
void ESP_AI_NET::feedWsStream(const uint8_t *data, size_t len)
{
	// append
	wsStreamBuffer.insert(wsStreamBuffer.end(), data, data + len);

	size_t idx = 0;
	while (true)
	{
		// 至少需要 2 字节头
		if (wsStreamBuffer.size() - idx < 2)
			break;

		uint8_t b1 = wsStreamBuffer[idx];
		uint8_t b2 = wsStreamBuffer[idx + 1];

		bool fin = (b1 & 0x80) != 0;
		uint8_t opcode = b1 & 0x0F;
		bool mask = (b2 & 0x80) != 0;
		uint64_t payloadLen = (uint64_t)(b2 & 0x7F);

		// pos 指向头后第一个字节（可能是扩展长度或 mask key）
		size_t pos = idx + 2;

		// 处理扩展长度
		if (payloadLen == 126)
		{
			// 需要 2 个字节
			if (wsStreamBuffer.size() - idx < (pos - idx) + 2)
				break; // 等待
			payloadLen = (uint64_t(wsStreamBuffer[pos]) << 8) | uint64_t(wsStreamBuffer[pos + 1]);
			pos += 2;
		}
		else if (payloadLen == 127)
		{
			// 需要 8 个字节
			if (wsStreamBuffer.size() - idx < (pos - idx) + 8)
				break;
			payloadLen = 0;
			for (int i = 0; i < 8; ++i)
				payloadLen = (payloadLen << 8) | (uint64_t)wsStreamBuffer[pos + i];
			pos += 8;
		}

		// mask key
		uint8_t maskKey[4] = {0, 0, 0, 0};
		if (mask)
		{
			if (wsStreamBuffer.size() - idx < (pos - idx) + 4)
				break; // 等待完整 mask key
			for (int i = 0; i < 4; ++i)
				maskKey[i] = wsStreamBuffer[pos + i];
			pos += 4;
		}

		// 检查是否到齐 payload
		if (wsStreamBuffer.size() - idx < (pos - idx) + payloadLen)
			break; // 等待更多字节

		// payload 起点
		size_t payloadStart = pos;

		// Debug: 打印头信息（太多可以注释）
		Serial.printf("[feedWsStream] idx=%u FIN=%d OPCODE=0x%02X MASK=%d PAYLOAD=%llu HEADERLEN=%u\n",
					  (unsigned)idx, fin ? 1 : 0, opcode, mask ? 1 : 0,
					  (unsigned long long)payloadLen, (unsigned)(pos - idx));

		// 提取并去 mask（如果有）
		std::vector<uint8_t> payload;
		if (payloadLen > 0)
		{
			payload.reserve((size_t)payloadLen);
			for (uint64_t i = 0; i < payloadLen; ++i)
			{
				uint8_t byte = wsStreamBuffer[payloadStart + i];
				if (mask)
					byte ^= maskKey[i % 4];
				payload.push_back(byte);
			}
		}

		// 处理帧：支持文本、二进制、continuation、control frames
		if (opcode == 0x1) // 文本
		{
			std::string s((const char *)payload.data(), payload.size());
			LOG_I("[WS] 文本帧, len=%llu: %s", (unsigned long long)payloadLen, s.c_str());
			// TODO: 将 s 交由上层处理（队列、回调等）
		}
		else if (opcode == 0x2) // 二进制
		{
			LOG_I("[WS] 二进制帧, len=%llu", (unsigned long long)payloadLen);
			// TODO: handleBinary(payload.data(), payload.size());
		}
		else if (opcode == 0x0) // continuation
		{
			LOG_I("[WS] continuation 帧, len=%llu, FIN=%d", (unsigned long long)payloadLen, fin ? 1 : 0);
			// TODO: 如果你需要支持跨帧拼接（例如分片的文本/二进制），在这里拼接缓存并在 FIN==1 时处理
		}
		else if (opcode == 0x8) // close
		{
			LOG_I("[WS] close 帧");
		}
		else if (opcode == 0x9) // ping
		{
			LOG_I("[WS] ping 帧");
			// 发送 pong 帧
			// ...
			// 构造 pong 帧
			uint8_t pongFrame[2] = {0x8A, 0x00}; // FIN + pong opcode + 0 payload
			sendBIN(pongFrame, 2, EAN_WSop_binary);			 // 使用你已有的 TCP 发送函数
		}
		else if (opcode == 0xA) // pong
		{
			LOG_I("[WS] pong 帧");
		}
		else
		{
			LOG_I("[WS] 未知/保留 opcode=0x%02X, len=%llu", opcode, (unsigned long long)payloadLen);
		}

		// 跳过已处理的字节，继续解析下一帧
		idx = payloadStart + payloadLen;
	}

	// 清除已解析部分
	if (idx > 0)
	{
		std::vector<uint8_t> remain(wsStreamBuffer.begin() + idx, wsStreamBuffer.end());
		wsStreamBuffer.swap(remain);
	}
}

void serialDataTask(void *arg)
{
	SerialDataTaskContext *ctx = static_cast<SerialDataTaskContext *>(arg);
	if (ctx == nullptr)
	{
		LOG_E("[Error] serialDataTask ctx is null!");
		vTaskDelete(NULL);
		return;
	}

	LOG_I("开始监听串口数据");

	String lineBuffer = "";
	while (1)
	{
		while (ctx->serial_4g->available())
		{
			char c = ctx->serial_4g->read();

			// 🔹 普通行逻辑
			if (c == '\r' || c == '\n')
			{
				if (lineBuffer.length() > 0)
				{
					xSemaphoreTake(ctx->serialDataMutex, portMAX_DELAY);
					lineBuffer.trim();
					*(ctx->serialData) += lineBuffer + "\n";
					xSemaphoreGive(ctx->serialDataMutex);

					// ---------- 替换这里的 rtcp 处理分支 ----------
					if (lineBuffer.startsWith("+MIPURC: \"rtcp\""))
					{
						// 不要 trim()！不要对二进制区域做任何文本处理
						// 直接用索引查找逗号位置（String 的 indexOf 对 ASCII header 是安全的）
						int firstComma = lineBuffer.indexOf(',');
						int secondComma = lineBuffer.indexOf(',', firstComma + 1);
						int thirdComma = lineBuffer.indexOf(',', secondComma + 1);

						if (secondComma > 0 && thirdComma > 0)
						{
							String lenStr = lineBuffer.substring(secondComma + 1, thirdComma);
							int dataLen = lenStr.toInt();
							LOG_I("[RTCP] 检测到 TCP 数据长度: %d", dataLen);

							// 直接用二进制安全方式把 lineBuffer 中第三逗号之后的“已读字节”拷贝出来
							int remainCount = lineBuffer.length() - (thirdComma + 1); // 可能为 0
							uint8_t *buffer = (uint8_t *)malloc(dataLen > 0 ? dataLen : 1);
							if (!buffer)
							{
								LOG_E("[RTCP] malloc 失败");
								lineBuffer = "";
								continue;
							}
							int copied = 0;

							// ① 从 lineBuffer 中拷贝已读的部分（按二进制拷贝每个 char）
							for (int i = 0; i < remainCount && copied < dataLen; ++i)
							{
								// 用 charAt 获取原始字节并转换为 uint8_t（保证不做任何编码转换）
								char ch = lineBuffer.charAt(thirdComma + 1 + i);
								buffer[copied++] = (uint8_t)ch;
							}

							// ② 从串口继续读取剩余字节（直到 dataLen）
							unsigned long startMs = millis();
							while (copied < dataLen)
							{
								if (ctx->serial_4g->available())
								{
									int v = ctx->serial_4g->read();
									if (v >= 0)
										buffer[copied++] = (uint8_t)v;
								}
								else
								{
									// 超时保护：如果长时间没数据，避免死循环（你可以根据需求调大/调小）
									if (millis() - startMs > 5000)
									{
										LOG_E("[RTCP] 等待数据超时 已读取=%d 目标=%d", copied, dataLen);
										break;
									}
									vTaskDelay(1);
								}
							}

							// Debug: 打印 hex 头 及前若干字节，确认对齐（你可以注释掉以节省串口时间）
							Serial.printf("[RTCP] 收到数据:\n");
							for (int i = 0; i < copied; ++i)
							{
								Serial.printf("%02X ", buffer[i]);
								if ((i & 0x1F) == 0x1F)
									Serial.println();
							}
							Serial.println();

							// 调用你的流解析器（替换旧的 wsOnData）
							ctx->feedWsStream(buffer, copied); // 或 wsOnData(buffer, copied)（建议使用 feedWsStream）

							free(buffer);
						}

						// 清空缓存行（注意：不要再 trim）
						lineBuffer = "";
						continue;
					}
					// 你的原逻辑
					LOG_I("---%s", lineBuffer.c_str());
					lineBuffer = "";
				}
			}
			else
			{
				// 🔹 保持原行拼接逻辑
				lineBuffer += c;
			}
		}

		// 🔹 原来的拆包保护逻辑保留
		if (lineBuffer.length() > 1024)
		{
			xSemaphoreTake(ctx->serialDataMutex, portMAX_DELAY);
			lineBuffer.trim();
			*(ctx->serialData) += lineBuffer + "\n";
			xSemaphoreGive(ctx->serialDataMutex);

			LOG_I("---- %s", lineBuffer.c_str());
			lineBuffer = "";
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

EAN_WStype_t mapOpcodeToWStype(uint8_t opcode, bool fin)
{
	switch (opcode)
	{
	case 0x1: // text
		return fin ? EAN_WStype_TEXT : EAN_WStype_FRAGMENT_TEXT_START;
	case 0x2: // binary
		return fin ? EAN_WStype_BIN : EAN_WStype_FRAGMENT_BIN_START;
	case 0x0: // continuation
		return fin ? EAN_WStype_FRAGMENT_FIN : EAN_WStype_FRAGMENT;
	case 0x8:
		return EAN_WStype_DISCONNECTED;
	case 0x9:
		return EAN_WStype_PING;
	case 0xA:
		return EAN_WStype_PONG;
	default:
		return EAN_WStype_ERROR;
	}
}

EAN_TCPDataModel getTCPDataModel_impl()
{
	return ESP_AI_NET::instance->tcpDataModel;
}

// void wsOnData_impl(EAN_WStype_t type, uint8_t *payload, size_t length)
// {
// 	auto self = ESP_AI_NET::instance;
// 	if (self == nullptr)
// 		return;

// 	// Serial.printf("[WS] 收到类型:%d, 长度:%d\n", type, length);
// 	switch (type)
// 	{
// 	case EAN_WStype_CONNECTED:
// 		Serial.printf("[WS] 连接成功\n");
// 		break;
// 	case EAN_WStype_DISCONNECTED:
// 		Serial.printf("[WS] 断开连接\n");
// 		break;
// 	case EAN_WStype_PING:
// 		Serial.printf("[WS] PING\n");
// 		break;
// 	case EAN_WStype_PONG:
// 		Serial.printf("[WS] PONG\n");
// 		break;
// 	case EAN_WStype_ERROR:
// 		Serial.printf("[WS] ERROR\n");
// 		break;
// 	case EAN_WStype_TEXT:
// 		Serial.printf("[WS] 收到文本类型, 长度:%d, 内容:%s\n", length,  String((const char*)payload).c_str());
// 		Serial.println((const char*)payload);
// 		break;
// 	case EAN_WStype_BIN:
// 	{
// 		String hexStr;
// 		for (size_t i = 0; i < length; i++)
// 		{
// 			char buf[4];
// 			sprintf(buf, "%02X ", payload[i]);
// 			hexStr += buf;
// 			// 每 16 字节换行，方便阅读
// 			if ((i + 1) % 16 == 0)
// 				hexStr += "\n";
// 		}
// 		LOG_I("[TCP] 二进制数据 (%d 字节):\n%s", length, hexStr.c_str());
// 		break;
// 	}
// 	case EAN_WStype_FRAGMENT_TEXT_START:
// 		Serial.printf("[WS] EAN_WStype_FRAGMENT_TEXT_START \n");
// 		break;
// 	case EAN_WStype_FRAGMENT_BIN_START:
// 		Serial.printf("[WS] EAN_WStype_FRAGMENT_BIN_START \n");
// 		break;
// 	case EAN_WStype_FRAGMENT:
// 		Serial.printf("[WS] EAN_WStype_FRAGMENT \n");
// 		break;
// 	case EAN_WStype_FRAGMENT_FIN:
// 		Serial.printf("[WS] EAN_WStype_FRAGMENT_FIN \n");
// 		break;
// 	default:
// 		Serial.printf("[WS] 未知类型:%d, 长度:%d\n", type, length);
// 		break;
// 	}
// }

// void wsOnData_impl(uint8_t *payload, size_t length)
// {
// 	if (!payload || length == 0)
// 		return;

// 	// 判断是否是文本（ASCII 可打印 + 控制字符 \r \n \t）
// 	bool isText = std::all_of(payload, payload + length, [](uint8_t ch)
// 							  { return (ch >= 32 && ch <= 126) || ch == '\r' || ch == '\n' || ch == '\t'; });

// 	if (isText)
// 	{
// 		// 文本处理
// 		String text((char *)payload, length);
// 		Serial.print("[WS] 收到文本类型: ");
// 		Serial.println(text);
// 	}
// 	else
// 	{
// 		// 二进制 WS 解析
// 		size_t offset = 0;
// 		while (offset + 2 <= length)
// 		{
// 			uint8_t fin_opcode = payload[offset];
// 			uint8_t mask_len = payload[offset + 1];

// 			bool fin = fin_opcode & 0x80;
// 			uint8_t opcode = fin_opcode & 0x0F;
// 			bool masked = mask_len & 0x80;
// 			uint64_t payload_len = mask_len & 0x7F;
// 			size_t header_len = 2;

// 			if (payload_len == 126)
// 			{
// 				if (offset + 4 > length)
// 					break;
// 				payload_len = (payload[offset + 2] << 8) | payload[offset + 3];
// 				header_len = 4;
// 			}
// 			else if (payload_len == 127)
// 			{
// 				if (offset + 10 > length)
// 					break;
// 				payload_len = 0;
// 				for (int i = 0; i < 8; i++)
// 					payload_len = (payload_len << 8) | payload[offset + 2 + i];
// 				header_len = 10;
// 			}

// 			uint8_t mask[4] = {0};
// 			if (masked)
// 			{
// 				if (offset + header_len + 4 > length)
// 					break;
// 				memcpy(mask, &payload[offset + header_len], 4);
// 				header_len += 4;
// 			}

// 			if (offset + header_len + payload_len > length)
// 				break; // 不完整帧，跳出

// 			uint8_t *framePayload = payload + offset + header_len;
// 			// 如果有 mask，解码
// 			if (masked)
// 			{
// 				for (size_t i = 0; i < payload_len; i++)
// 					framePayload[i] ^= mask[i % 4];
// 			}

// 			// 输出内容
// 			Serial.print("[WS] 收到二进制帧, opcode=");
// 			Serial.print(opcode);
// 			Serial.print(", 长度=");
// 			Serial.println(payload_len);

// 			// 这里可以根据 opcode 做不同处理
// 			// opcode=1 文本, opcode=2 二进制, etc.

// 			offset += header_len + payload_len;
// 			if (fin)
// 				break;
// 		}
// 	}
// }

void ESP_AI_NET::monitorSerial()
{
	ESP_AI_NET::instance = this;
	serialDataTask_ctx = {
		serial_4g,
		serialDataMutex,
		&serialData,
		&TCPConnectId,
		getTCPDataModel_impl,
		// feedWsStream,
	};


    serialDataTask_ctx.feedWsStream = [this](const uint8_t *data, size_t len)
    { 
		this->feedWsStream(data, len);
    };

	xTaskCreateStatic(serialDataTask, "serialDataTask", SERIAL_DATA_TASK_SIZE, &serialDataTask_ctx, 1, serialDataTaskStack, &serialDataTaskBuffer);
}

String ESP_AI_NET::mergeJsonFromHTTP(const String &raw)
{
	int idx = raw.indexOf("{");
	int idx2 = raw.indexOf("[");
	if (idx != -1)
	{
		String json = raw.substring(idx);
		return json;
	}

	if (idx2 != -1)
	{
		String json = raw.substring(idx2);
		return json;
	}
	return "";
}

/**
 * Base64编码
 * 作用：将二进制数据编码为Base64字符串
 * 必要性：WebSocket握手和认证需要Base64编码
 */
String ESP_AI_NET::base64Encode(const uint8_t *data, size_t length)
{
	size_t outLen;
	mbedtls_base64_encode(nullptr, 0, &outLen, data, length);

	char *encoded = new char[outLen + 1];
	mbedtls_base64_encode((unsigned char *)encoded, outLen + 1, &outLen, data, length);
	encoded[outLen] = '\0';

	String result = String(encoded);
	delete[] encoded;
	return result;
}

/**
 * 生成WebSocket密钥
 * 作用：生成握手所需的随机密钥
 * 必要性：WebSocket握手协议要求
 */
String ESP_AI_NET::generateWebSocketKey()
{
	uint8_t key[16];
	for (int i = 0; i < 16; i++)
	{
		key[i] = random(256);
	}
	return base64Encode(key, 16);
}

String ESP_AI_NET::sendATCommand(const String &command, int awaitTime, const String &endStr)
{
	// 检查互斥锁是否被占用
	TaskHandle_t holder = xSemaphoreGetMutexHolder(atExecMutex);
	if (holder != NULL)
	{
		const char *taskName = pcTaskGetName(holder);
		LOG_I("等待 AT 指令互斥锁被任务 [%s] 释放...", taskName ? taskName : "未知任务");
	}

	// 获取互斥锁
	if (xSemaphoreTake(atExecMutex, pdMS_TO_TICKS(10000)) != pdTRUE)
	{
		LOG_E("获取AT指令锁失败,发送数据失败.");
		return "";
	}

	// 清空串口缓存
	xSemaphoreTake(serialDataMutex, portMAX_DELAY);
	serialData = "";
	xSemaphoreGive(serialDataMutex);

	serialMaxAwaitTime = awaitTime;
	LOG_I("发送AT命令 ==> : %s", command.c_str());
	serial_4g->println(command);

	String localData, prevData;

	while (serialMaxAwaitTime > 0)
	{
		// 读取串口缓存
		xSemaphoreTake(serialDataMutex, portMAX_DELAY);
		localData = serialData;
		xSemaphoreGive(serialDataMutex);

		// 检查响应
		if (isResponseOK(localData, endStr))
			break;

		// 有新数据则延长一点等待时间
		if (prevData != localData)
		{
			serialMaxAwaitTime += 50;
			if (serialMaxAwaitTime > awaitTime * 2)
				serialMaxAwaitTime = awaitTime * 2;
			// LOG_I("收到新数据: %s", localData.c_str());
			prevData = localData;
		}

		serialMaxAwaitTime -= 10;
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}

	if (serialMaxAwaitTime <= 0)
		LOG_E("AT命令错误, %s", localData.c_str());

	// 释放互斥锁
	xSemaphoreGive(atExecMutex);

	return localData;
}

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
String ESP_AI_NET::GET(const String &host, const String &path)
{
	while (!networkReady)
	{
		LOG_E("网络未初始化完毕, 无法发出GET请求.");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	if (host.length() == 0)
	{
		LOG_E("URL不能为空");
		return "";
	}

	LOG_I("开始HTTP GET 请求: %s %s", host.c_str(), path.c_str());

	// 结束上一次的请求
	sendATCommand("AT+MHTTPTERM=0", 1000);
	sendATCommand("AT+MHTTPDEL=0", 1000);

	// 设置HTTP参数
	String cmd = String("AT+MHTTPCREATE=\"") + host + "\"";
	String res = sendATCommand(cmd, 1000);
	if (!isResponseOK(res))
	{
		LOG_E("创建HTTP连接失败: %s", res.c_str());
		return "";
	}

	// 发送GET请求
	String resReq = sendATCommand("AT+MHTTPREQUEST=0,1,0,\"" + path + "\"", 10000, "+MHTTPURC: \"content\",");
	if (!isResponseOK(resReq, "+MHTTPURC: \"content\","))
	{
		LOG_E("GET 请求失败: %s", resReq.c_str());
		return "";
	}
	return mergeJsonFromHTTP(resReq);
}

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
String ESP_AI_NET::POST(const String &host, const String &path, const String &params)
{
	while (!networkReady)
	{
		LOG_E("网络未初始化完毕, 无法发出POST请求.");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	if (host.length() == 0)
	{
		LOG_E("URL不能为空");
		return "";
	}

	LOG_I("开始HTTP POST 请求: %s %s", host.c_str(), path.c_str());

	// 结束上一次的请求
	sendATCommand("AT+MHTTPTERM=0", 1000);
	sendATCommand("AT+MHTTPDEL=0", 1000);

	// 设置HTTP参数
	String cmd = String("AT+MHTTPCREATE=\"") + host + "\"";
	String res = sendATCommand(cmd, 1000);
	if (!isResponseOK(res))
	{
		LOG_E("创建HTTP连接失败: %s", res.c_str());
		return "";
	}

	// 设置HTTP头信息
	String resHeader = sendATCommand("AT+MHTTPHEADER=0,0,0,\"Content-Type: application/json\"", 1000);
	if (!isResponseOK(resHeader))
	{
		LOG_E("设置 POST 头信息失败: %s", resHeader.c_str());
		return "";
	}

	// 设置HTTP参数
	String resParams = sendATCommand("AT+MHTTPCONTENT=0,0,0,\"" + params + "\"", 1000);
	if (!isResponseOK(resParams))
	{
		LOG_E("设置 POST 参数失败: %s", resParams.c_str());
		return "";
	}

	// 发送请求
	String resReq = sendATCommand("AT+MHTTPREQUEST=0,2,0,\"/devices/add\"", 10000, "+MHTTPURC: \"content\",");
	if (!isResponseOK(resReq, "+MHTTPURC: \"content\","))
	{
		LOG_E("POST 请求失败: %s", resReq.c_str());
		return "";
	}
	return mergeJsonFromHTTP(resReq);
}

/**
 * WebSocket 请求
 * @param host 主机地址, 例如: api.espai.fun
 * @param path 请求路径, 例如: /connect_espai_node?param=xxx
 *
 * 涉及下面 AT 命令
 * AT+MIPCLOSE=0
 * AT+MIPOPEN=0,"TCP","120.237.117.213",9700
 * AT+MIPMODE=0,1  // 进入透传模式
 * // 升级为 WebSocket
 * GET /connect_espai_node?param=xxx HTTP/1.1\r\nHost: 120.237.117.213:9700\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ZYvFZbM7K1XzDp8n6SYnNQ==\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol:arduino\r\n\r\n
 */
bool ESP_AI_NET::connectWS(const String &host, const int &port, const String &path)
{
	while (!networkReady)
	{
		LOG_E("网络未初始化完毕, 无法发出 WS 请求.");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	if (host.length() == 0)
	{
		LOG_E("URL不能为空");
		return false;
	}

	LOG_I("开始 WS 请求: %s%d%s", host.c_str(), port, path.c_str());

	if (TCPConnectId != 999)
	{
		// 结束上一次的请求
		sendATCommand("AT+MIPCLOSE=" + String(TCPConnectId), 1000);
	}
	else
	{
		String res = sendATCommand("AT+MIPSTATE?", 1000);
		TCPConnectId = getFirstInitialChannel(res);
		LOG_I("获取到可用 TCP 通道: %d", TCPConnectId);
	}

	// 打开连接
	String cmd = String("AT+MIPOPEN=" + String(TCPConnectId) + ",\"TCP\",\"") + host + "\"," + String(port);
	String res = sendATCommand(cmd, 5000);
	if (!isResponseOK(res))
	{
		LOG_E("打开TCP连接失败: %s", res.c_str());
		return false;
	}

	setTCPDataModel(EAN_TCP_MODEL_TRANSPARENT);

	// 升级为 WebSocket
	String wsKey = generateWebSocketKey();
	String wsUpgradeCmd = String("GET ") + path + " HTTP/1.1\r\nHost: " + host + ":" + String(port) + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + wsKey + "\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol:arduino\r\n";
	String resUpgrade = sendATCommand(wsUpgradeCmd, 10000, "Sec-WebSocket-Protocol: arduino");
	if (!isResponseOK(resUpgrade, "Sec-WebSocket-Protocol: arduino"))
	{
		LOG_E("WebSocket 升级失败: %s", resUpgrade.c_str());
		return false;
	}

	// 设置连接状态
	setWSState(EAN_WS_CONNECTING);
	LOG_I("WebSocket 连接成功");
	setTCPDataModel(EAN_TCP_MODEL_DATA);
	return true;
}

/**
 * 向 WS 服务发送二进制文件
 *
 * 主函数：把 data(len) 按 WS_CHUNK_SIZE 拆包并发送（文本模式使用 opcode = 0x1）
 * 返回 true 表示成功发送所有片段，false 表示未发送（例如互斥超时/串口失败）
 *
 * sendBIN(bin.data(), bin.size(), 0x2);
 */
bool ESP_AI_NET::sendBIN(const uint8_t *data, size_t len, uint8_t opcodeFirst)
{
	if (getWSState() != EAN_WS_CONNECTING)
	{
		LOG_E("WS 服务未连接,无法发送数据.");
		return false;
	}

	if (!data || len == 0)
	{
		LOG_E("未传入任何数据,发送数据失败.");
		return false;
	}

	setTCPDataModel(EAN_TCP_MODEL_TRANSPARENT);
	Serial.println("开始发送数据");
	size_t offset = 0;
	bool first = true;
	bool ok = true;

	while (offset < len)
	{
		size_t remain = len - offset;
		size_t chunk = (remain > WS_CHUNK_SIZE) ? WS_CHUNK_SIZE : remain;

		uint8_t opcode = first ? opcodeFirst : EAN_WSop_continuation; // continuation for subsequent frames
		bool fin = (offset + chunk >= len);

		// 构造帧（堆上构造返回 vector）
		std::vector<uint8_t> frame = build_ws_frame_chunk(data + offset, chunk, opcode, fin);

		// 发送：先尝试写入全部字节
		size_t toSend = frame.size();
		const uint8_t *p = frame.data();
		size_t sent = 0;
		// Serial.write 会一次尽量写入，做循环写，防护性更强
		while (sent < toSend)
		{
			size_t wrote = serial_4g->write(p + sent, toSend - sent);
			if (wrote == 0)
			{
				// 写入失败或缓冲区被占满，短暂等待再重试（可调整策略）
				vTaskDelay(pdMS_TO_TICKS(5));
				// 小次数重试策略（防止无限卡住），这里简单重试几次
				static int retry = 0;
				retry++;
				if (retry > 200)
				{
					ok = false;
					break;
				} // 超过重试次数认为失败
				continue;
			}
			sent += wrote;
		}

		// 等待发送完成（可选）
		serial_4g->flush();

		if (!ok)
			break;

		offset += chunk;
		first = false;
	}
	setTCPDataModel(EAN_TCP_MODEL_DATA);
	return ok;
}

/**
 *  向 WS 服务发送文本
 *  bool ok = sendTXT("...很长很长的文本...");
 *  if (!ok) {
 *     Serial.println("发送失败");
 *  }
 */
bool ESP_AI_NET::sendTXT(const String &txt, uint8_t opcodeFirst)
{
	// 将 String 内容转换到堆上的 buffer，再调用上面的函数
	size_t len = txt.length();
	if (len == 0)
		return false;
	std::vector<uint8_t> buf;
	buf.reserve(len);
	for (size_t i = 0; i < len; ++i)
		buf.push_back((uint8_t)txt[i]);
	LOG_I("向 WS 服务发送: %s", txt.c_str());
	return sendBIN(buf.data(), buf.size(), opcodeFirst);
}

int ESP_AI_NET::getFirstInitialChannel(const String &response)
{
	int firstID = -1;
	int startIndex = 0;

	while (true)
	{
		// 查找下一行
		int endIndex = response.indexOf('\n', startIndex);
		if (endIndex == -1)
			break; // 没有更多行了

		String line = response.substring(startIndex, endIndex);
		line.trim(); // 去掉前后空白

		if (line.startsWith("+MIPSTATE:") && line.indexOf("INITIAL") != -1)
		{
			// 提取通道号
			int commaIndex = line.indexOf(',');
			if (commaIndex != -1)
			{
				String idStr = line.substring(String("+MIPSTATE: ").length(), commaIndex);
				idStr.trim();
				firstID = idStr.toInt();
				break;
			}
		}

		startIndex = endIndex + 1;
	}

	return firstID; // 如果没找到返回 -1
}

// void ESP_AI_NET::onEvent(WebSocketClientEvent cbEvent)
// {
// 	wsOnDataCb = cbEvent;
// }

// void ESP_AI_NET::wsOnData(EAN_WStype_t type, uint8_t *payload, size_t length)
// {
// 	Serial.print("length: ");
// 	Serial.println(length);
// }
