/**
 * * Copyright (c) 2026 Cyberware Workshop
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
 * 请注意：将 Cyberware Workshop 代码用于商业用途需要事先获得许可方的授权。
 * 删除与修改版权属于侵权行为，请尊重作者版权，避免产生不必要的纠纷。
 *
 * @author Cyberware Workshop Team
 * @date 2026
 * 
 * 蓝牙管理模块头文件
 * 
 * 负责蓝牙A2DP连接管理和状态回调
 * 
 */

#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <Arduino.h>
#include "BluetoothA2DPSink.h"

/**
 * 初始化蓝牙A2DP接收器
 * 
 * @param deviceName 蓝牙设备名称
 */
void initBluetooth(const char* deviceName);

/**
 * 获取蓝牙A2DP Sink对象指针
 * 
 * @return BluetoothA2DPSink对象指针
 */
BluetoothA2DPSink* getA2DPSink();

/**
 * 蓝牙连接状态回调函数
 */
void connection_state_changed(esp_a2d_connection_state_t state, void *ptr);

/**
 * 音频播放状态回调函数
 */
void audio_state_changed(esp_a2d_audio_state_t state, void *ptr);

/**
 * AVRC元数据回调函数
 */
void avrc_metadata_callback(uint8_t data1, const uint8_t *data2);

/**
 * 获取蓝牙连接状态
 * 
 * @return true=已连接, false=未连接
 */
bool isBluetoothConnected();

/**
 * 获取蓝牙连接状态与UI所需状态快照
 *
 * @param connected 是否已连接
 * @param playing 是否播放中
 * @param volume 当前音量
 * @param title 标题输出缓冲区
 * @param titleSize 标题缓冲区大小
 * @param artist 艺术家输出缓冲区
 * @param artistSize 艺术家缓冲区大小
 * @param album 专辑输出缓冲区
 * @param albumSize 专辑缓冲区大小
 */
void getBluetoothUiSnapshot(bool *connected, bool *playing, uint8_t *volume, char *title, size_t titleSize, char *artist, size_t artistSize, char *album, size_t albumSize);

/**
 * 获取音频播放状态
 *
 * @return true=播放中, false=暂停
 */
bool isAudioPlaying();

/**
 * 恢复出厂设置（清除所有配对设备）
 */
void factoryReset();

/**
 * 播放/暂停切换
 * 如果当前正在播放则暂停，如果暂停则播放
 */
void togglePlayPause();

/**
 * 播放
 */
void playMusic();

/**
 * 暂停
 */
void pauseMusic();

/**
 * 下一曲
 */
void nextTrack();

/**
 * 上一曲
 */
void previousTrack();

/**
 * 设置音量 (使用A2DP AVRCP协议)
 *
 * @param volume 音量值 (0-127)
 */
void setVolume(uint8_t volume);

/**
 * 获取当前音量
 *
 * @return 音量值 (0-127)
 */
uint8_t getVolume();

/**
 * 增加音量
 *
 * @param step 增加的步进值 (默认约10%)
 */
void increaseVolume(uint8_t step = 13);

/**
 * 减少音量
 *
 * @param step 减少的步进值 (默认约10%)
 */
void decreaseVolume(uint8_t step = 13);

/**
 * 设置元数据更新回调函数
 *
 * @param callback 回调函数指针，参数为 (title, artist, album)
 */
void setMetadataCallback(void (*callback)(const char*, const char*, const char*));

/**
 * 设置曲目切换回调函数
 *
 * @param callback 回调函数指针，参数为 (isNext: true=下一曲, false=上一曲)
 */
void setTrackChangeCallback(void (*callback)(bool isNext));

/**
 * 设置音量变化回调函数
 *
 * @param callback 回调函数指针，参数为 (volume: 0-127)
 */
void setVolumeChangeCallback(void (*callback)(uint8_t volume));

/**
 * 获取当前歌曲标题
 *
 * @return 歌曲标题字符串
 */
const char* getCurrentTitle();

/**
 * 获取当前艺术家
 *
 * @return 艺术家名称字符串
 */
const char* getCurrentArtist();

/**
 * 获取当前专辑
 *
 * @return 专辑名称字符串
 */
const char* getCurrentAlbum();

#endif // BLUETOOTH_MANAGER_H
