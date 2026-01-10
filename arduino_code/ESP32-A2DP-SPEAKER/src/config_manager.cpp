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
 * 配置管理模块实现
 * 
 * 负责蓝牙配对信息的保存和加载
 * 
 */

#include "config_manager.h"
#include "Preferences.h"

// Preferences对象
static Preferences preferences;

/**
 * 保存蓝牙配置信息
 */
void saveBluetoothConfig() {
  preferences.begin("bluetooth", false);
  preferences.putBool("paired", true);
  preferences.end();
}

/**
 * 加载蓝牙配置信息
 */
bool loadBluetoothConfig() {
  preferences.begin("bluetooth", true);
  bool paired = preferences.getBool("paired", false);
  preferences.end();
  return paired;
}

/**
 * 清除蓝牙配置信息
 */
void clearBluetoothConfig() {
  preferences.begin("bluetooth", false);
  preferences.clear();
  preferences.end();
}
