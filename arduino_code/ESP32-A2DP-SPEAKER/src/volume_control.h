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
 * 音量控制模块头文件
 *
 * 音量控制使用A2DP AVRCP协议
 * 此模块提供向后兼容的接口
 */

#ifndef VOLUME_CONTROL_H
#define VOLUME_CONTROL_H

#include <Arduino.h>

/**
 * 初始化音量控制模块
 * (当前使用A2DP音量控制，无需特殊初始化)
 */
void initVolumeControl();

/**
 * 更新音量
 * (当前使用A2DP音量控制，由蓝牙模块自动处理)
 */
void updateVolume();

/**
 * 获取当前音量值
 *
 * @return 音量值 (0.0 - 1.0)
 */
float getCurrentVolume();

#endif // VOLUME_CONTROL_H

