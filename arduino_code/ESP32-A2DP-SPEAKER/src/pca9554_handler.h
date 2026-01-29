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
 * PCA9554 IO扩展芯片处理模块头文件
 * 
 * 负责PCA9554 IO变化检测和蓝牙播放控制
 * 
 */

#ifndef PCA9554_HANDLER_H
#define PCA9554_HANDLER_H

#include <Arduino.h>

/**
 * 初始化PCA9554模块
 * 配置I2C、中断引脚和IO扩展芯片
 * 
 * @return true=初始化成功, false=初始化失败
 */
bool initPCA9554Handler();

/**
 * 更新PCA9554状态
 * 在主循环中调用，检查IO变化并执行相应操作
 * @return true=有按钮操作, false=无操作
 */
bool updatePCA9554();

#endif // PCA9554_HANDLER_H

