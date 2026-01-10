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
 * LED控制模块头文件
 * 
 * 负责WS2812 RGB LED状态指示
 * 
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

/**
 * 初始化LED控制模块
 */
void initLedControl();

/**
 * 更新LED状态显示
 * 应在主循环中定期调用
 * 
 * @param connected 蓝牙连接状态
 * @param playing 音频播放状态
 */
void updateRgbLed(bool connected, bool playing);

#endif // LED_CONTROL_H

