/***************************************************
Copyright (c) 2020 Luis Llamas
(www.luisllamas.es)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <http://www.gnu.org/licenses
****************************************************/

#ifndef _FACE_h
#define _FACE_h

#include <Arduino.h>
#include "Common.h"
#include "Animations.h"
#include "EyePresets.h"
#include "Eye.h"
#include "FaceExpression.h"
#include "FaceBehavior.h"
#include "LookAssistant.h"
#include "BlinkAssistant.h"

class Face
{
public:
    /* eye_size:眼睛大小 *** screenType:屏幕类型 *** clock:时钟引脚 *** data:数据引脚 *** rotation:旋转角度 *** txt_color:文字颜色 *** bg_color:背景颜色 *** eye_color:眼睛颜色 ***/
    Face(uint8_t eye_size = 4, const String &screenType = "096", uint8_t clock = 38, uint8_t data = 39, int rotation = 0, uint16_t txt_color = 0x06FF, uint16_t bg_color = 0x0000, uint16_t eye_color = 0x06FF);
    ~Face();

    // 表情对象
    Eye LeftEye;
    Eye RightEye;
    BlinkAssistant Blink;
    FaceBehavior Behavior;
    FaceExpression Expression;

    // 顶部状态栏显示
    void ShowNotification(const String &notification);
    // 设置表情
    void SetEmotion(const String &emotion);
    // 设置底部滚动消息
    void SetChatMessage(String Message);
    // 设置电池电量,0xFF为正在充电
    void SetBatLevel(uint8_t level);
    // 设置音量
    void SetVolume(uint8_t volume);
    // 设置是否只显示一行文字，其他的都不渲染，仅仅显示 notification_message_ 文字。并且居中显示
    void OnlyShowNotification(bool only_show_notification);

    int width() const { return width_; }
    int height() const { return height_; }

    void Update();

private:
    int width_ = 0;
    int height_ = 0;
    uint16_t bg_color_ = 0x0005;
    uint16_t txt_color_ = 0xFFFF; // 白色

    String screenType_ = "096";
    uint8_t bat_level_ = 0;
    uint8_t volume_ = 0;

    bool only_show_notification_ = false; 
    String chat_message_;
    String notification_message_;
};

#endif