/***************************************************
Copyright (c) 2020 Luis Llamas
(www.luisllamas.es)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <http://www.gnu.org/licenses
****************************************************/
#include <WiFi.h>
#include <Battery.h>
#include "Face.h"

#if SCREEN_TYPE == 1
// OLED基类指针
U8G2 *u8g2;

#else /******************else******************/

TFT_eSPI tft = TFT_eSPI();				// tft instance
TFT_eSprite sprite = TFT_eSprite(&tft); // 创建双缓冲区
// TFT基类指针
U8g2_for_TFT_eSPI *u8g2;
#endif

/* eye_size:眼睛大小 *** screenType:屏幕类型 *** clock:时钟引脚 *** data:数据引脚 *** rotation:旋转角度 *** txt_color:文字颜色 *** bg_color:背景颜色 *** eye_color:眼睛颜色 ***/
Face::Face(uint8_t eye_size, const String &screenType, uint8_t clock, uint8_t data, int rotation, uint16_t txt_color, uint16_t bg_color, uint16_t eye_color)
	: LeftEye(*this), RightEye(*this), Blink(*this), Behavior(*this), Expression(*this)
{
	bg_color_ = bg_color;
	txt_color_ = txt_color;
	screenType_ = screenType;
#if SCREEN_TYPE == 1
	if (screenType == "091")
		u8g2 = new U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C(rotation == 0 ? U8G2_R0 : U8G2_R2, /* clock=*/clock, /* data= */ data, /* reset=*/U8X8_PIN_NONE); 
	else
		u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(rotation == 0 ? U8G2_R0 : U8G2_R2, /* reset=*/U8X8_PIN_NONE, /* clock=*/clock, /* data= */ data);
	u8g2->begin();
	// 必须启用，否则乱码
	u8g2->enableUTF8Print();

	width_ = u8g2->getWidth();
	height_ = u8g2->getHeight();
#else /******************else******************/
	tft.begin();
	tft.setRotation(rotation / 90);
	width_ = tft.width();
	height_ = tft.height();

	sprite.createSprite(width_, height_); // 创建双缓冲区
	u8g2 = new U8g2_for_TFT_eSPI();		  // create u8g2 procedures
	u8g2->begin(sprite);				  // connect u8g2 procedures to TFT_eSPI
	u8g2->setBackgroundColor(bg_color);
#endif
	u8g2->setFontMode(0);		   // use u8g2 transparent mode
	u8g2->setFontDirection(0);	   // left to right (this is default)
	u8g2->setDrawColor(txt_color); // apply color to u8g2 procedures

	// 表情初始化
	if (screenType == "096_2")
	{
		LeftEye.CenterX = width_ / 2;
		LeftEye.CenterY = height_ / 2;
		Expression.SetFaceSize(eye_size, 50);
	}else if(screenType == "240*240"){   
		LeftEye.CenterX = width_ * 3 / 10;
		LeftEye.CenterY = height_ * 5 / 9;
		Expression.SetFaceSize(8, 8);
	}
	else
	{
		LeftEye.CenterX = width_ * 3 / 9;
		LeftEye.CenterY = height_ * 5 / 9;
		Expression.SetFaceSize(eye_size, 0);
	}
	LeftEye.Color = eye_color;
	LeftEye.IsMirrored = true;
	RightEye.Color = eye_color;
	RightEye.CenterX = width_ * 6 / 9;
	RightEye.CenterY = height_ * 5 / 9;

	Expression.ClearVariations();
	Behavior.Clear();
	Behavior.Timer.Start();
	Behavior.SetEmotion(eEmotions::Normal, 2.0);
}

Face::~Face()
{
	delete u8g2;
	u8g2 = NULL;
	chat_message_ = "";
	notification_message_ = "";
}

// 字符串中包含字符的宽度计算函数
static int calculateTextWidth(const String &status)
{
	int totalWidth = 0;
	size_t pos = 0;
	int8_t charWidth = u8g2->getMaxCharWidth() - 1;
	int8_t EcharHeight = charWidth / 2;

	while (pos < status.length())
	{
		// 检查当前字符是否为中文字符（UTF-8编码的中文字符通常占用3个字节）
		if ((status[pos] & 0xF0) == 0xE0)
		{
			// 中文字符宽度为16
			totalWidth += charWidth;
			pos += 3; // 跳过3个字节
		}
		else
		{
			// 英文字符和符号宽度为8
			totalWidth += EcharHeight;
			pos += 1; // 跳过1个字节
		}
	}

	return totalWidth;
}

void Face::OnlyShowNotification(bool only_show_notification)
{
	only_show_notification_ = only_show_notification;
}

void Face::ShowNotification(const String &notification)
{
	notification_message_ = notification;
	notification_message_.trim(); // 消除非可见字符
}

static void DrawNotification(const String &notification, uint16_t txt_color, int width, int height, bool only_show_notification_)
{
	u8g2->setFont(u8g2_font_wqy12_t_gb2312); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
	u8g2->setDrawColor(txt_color);			 // apply color to u8g2 procedures

	if (only_show_notification_ == true)
	{
		// 居中显示
		u8g2->setCursor((width - calculateTextWidth(notification)) / 2, height / 2); 
	}
	else
	{
		// 左上角
		u8g2->setCursor(0, u8g2->getMaxCharHeight() - 1);
	}

	u8g2->print(notification.c_str());
}

void Face::SetEmotion(const String &emotion)
{
	// 表情设置
	if (emotion == "无情绪" || emotion == "默认")
	{
		Behavior.GoToEmotion(eEmotions::Normal);
	}
	else if (emotion == "快乐")
	{
		Behavior.GoToEmotion(eEmotions::Happy);
	}
	else if (emotion == "伤心")
	{
		Behavior.GoToEmotion(eEmotions::Sad);
	}
	else if (emotion == "愤怒")
	{
		Behavior.GoToEmotion(eEmotions::Furious);
	}
	else if (emotion == "意外")
	{
		Behavior.GoToEmotion(eEmotions::Surprised);
	}
	else if (emotion == "专注")
	{
		Behavior.GoToEmotion(eEmotions::Focused);
	}
	else if (emotion == "发愁")
	{
		Behavior.GoToEmotion(eEmotions::Worried);
	}
	else if (emotion == "懊恼")
	{
		Behavior.GoToEmotion(eEmotions::Frustrated);
	}
	else if (emotion == "困倦")
	{
		Behavior.GoToEmotion(eEmotions::Sleepy);
	}
	else if (emotion == "疑问")
	{
		Behavior.GoToEmotion(eEmotions::Suspicious);
	}
	else if (emotion == "恐惧")
	{
		Behavior.GoToEmotion(eEmotions::Scared);
	}
	else if (emotion == "敬畏")
	{
		Behavior.GoToEmotion(eEmotions::Awe);
	}
	else if (emotion == "说话中")
	{
		Behavior.GoToEmotion(eEmotions::Focused);
	}
	else if (emotion == "聆听中")
	{
		Behavior.GoToEmotion(eEmotions::Squint);
	}
	else
	{
		Behavior.GoToEmotion(eEmotions::Skeptic);
	}
}

void Face::SetChatMessage(String Message)
{
	Message.trim();			   // 消除非可见字符
	Message.replace("\n", ""); // 去掉换行符
	// 如果前面的还没显示完
	if (width_ * 5 > calculateTextWidth(chat_message_))
	{
		chat_message_ = chat_message_ + "   " + Message; // 换行显示
	}
	else
	{
		chat_message_ = Message;
	}
}

void Face::SetBatLevel(uint8_t level)
{
	bat_level_ = level;
}

void Face::SetVolume(uint8_t volume)
{
	if (100 < volume)
	{
		return;
	}
	volume_ = volume;
}

// 显示底部消息，从右至左滚动显示
static void ShowNotificationAtBottom(String &message, uint16_t txt_color, int width, int height)
{
	static int text_x_position = width / 4; // 文字初始位置在屏幕中间靠左一点
	static int text_length = 0;

	u8g2->setFont(u8g2_font_wqy12_t_gb2312); // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
	// 计算文字的总长度，区分字符和中文
	text_length = calculateTextWidth(message); // 获取文字宽度

	// 如果文字已经完全滚出屏幕，则重置位置
	if (text_x_position <= -text_length)
	{
		text_x_position = width / 4;
		text_length = 0; // 重置长度，以便下次计算
		message.clear();
	}

	// 绘制文字
	u8g2->setDrawColor(txt_color);				  // 应用颜色到u8g2过程
	u8g2->setCursor(text_x_position, height - 1); // start writing at this position
	u8g2->print(message.c_str());

	// 每次调用该函数时，更新文字的位置
#if SCREEN_TYPE == 1
	text_x_position -= 4; // 每次调用函数时，文字向左移动4个像素
#else					  /******************else******************/
	text_x_position -= 10; // 每次调用函数时，文字向左移动12个像素
#endif
}

// 绘制左上角wifi状态图标
static void ShowWifiStatus(int width)
{
#if SCREEN_TYPE == 1
	uint8_t proportion = 1;
#else /******************else******************/
	int8_t proportion = 2;
#endif

	u8g2->setFont(u8g2_font_siji_t_6x10);
	if (WIFI_MODE_STA == WiFi.getMode())
	{
		if (WiFi.isConnected())
		{
			int rssi = WiFi.RSSI();
			if (rssi > -50) // 强信号
			{
				u8g2->drawGlyph(width - 28 * proportion, 10 * proportion, 57872 + 10); // 强信号的图标
			}
			else if (rssi > -80) // 中等信号
			{
				u8g2->drawGlyph(width - 28 * proportion, 10 * proportion, 57872 + 9); // 中等信号的图标
			}
			else // 弱信号
			{
				u8g2->drawGlyph(width - 28 * proportion, 10 * proportion, 57872 + 8); // 弱信号的图标
			}
		}
		else
		{
			u8g2->drawGlyph(width - 28 * proportion, 10 * proportion, 57872 + 11);
		}
	}
	else if (WIFI_MODE_APSTA == WiFi.getMode())
	{
		// 热点
		u8g2->drawGlyph(width - 28 * proportion, 10 * proportion, 57520 + 3);
	}
	else
	{
		// 无状态
		u8g2->drawGlyph(width - 28 * proportion, 10 * proportion, 57872 + 15);
	}
}

static void ShowBatStatus(int width, uint8_t bat_level)
{
#if SCREEN_TYPE == 1
	uint8_t proportion = 1;
#else /******************else******************/
	int8_t proportion = 2;
#endif
	u8g2->setFont(u8g2_font_siji_t_6x10);

	if (0xFF == bat_level)
	{
		u8g2->drawGlyph(width - 12 * proportion, 10 * proportion, 57904 + 9);
	}
	else
	{
		u8g2->drawGlyph(width - 12 * proportion, 10 * proportion, 57922 + map(bat_level, 0, 100, 0, 9));
	}
}

static void ShowVolume(int width, uint8_t volume)
{
#if SCREEN_TYPE == 1
	uint8_t proportion = 1;
#else /******************else******************/
	int8_t proportion = 2;
#endif
	u8g2->setFont(u8g2_font_siji_t_6x10);
	if (5 > volume) // 绘制静音图标
	{
		u8g2->drawGlyph(width - 40 * proportion, 10 * proportion, 57424 + 2);
	}
	else
	{
		u8g2->drawGlyph(width - 40 * proportion, 10 * proportion, 57568 + map(volume, 5, 100, 3, 0));
	}
}

static void ShowEmotion(FaceBehavior &Behavior, Eye &LeftEye, Eye &RightEye, BlinkAssistant &Blink, String &screenType)
{
	// Update behavior
	Behavior.Update();
	Blink.Update();
	// Draw left eye
	LeftEye.Draw();
	// Draw right eye
	if (screenType != "096_2")
	{
		RightEye.Draw();
	}
}

void Face::Update()
{

#if SCREEN_TYPE == 1
	u8g2->clearBuffer();
#else /******************else******************/
	sprite.fillSprite(bg_color_);
#endif
	if (only_show_notification_)
	{
		// 绘制通知消息
		DrawNotification(notification_message_, txt_color_, width_, height_, only_show_notification_);
	}
	else
	{ 
		if (height_ > 32)
		{
			// 绘制表情
			ShowEmotion(Behavior, LeftEye, RightEye, Blink, screenType_);
		}

		if (screenType_ != "096_2")
		{
			// 绘制通知消息
			DrawNotification(notification_message_, txt_color_, width_, height_, only_show_notification_);
			// 绘制底部消息
			if (chat_message_.length())
			{
				ShowNotificationAtBottom(chat_message_, txt_color_, width_, height_);
			}

			// 绘制右上角wifi状态图标
			ShowWifiStatus(width_);

			// 绘制右上角电量图标
			ShowBatStatus(width_, bat_level_);

			// 绘制右上角音量图标
			ShowVolume(width_, volume_);
		}
	}

	// 将缓冲区内容显示到屏幕上
#if SCREEN_TYPE == 1
	u8g2->sendBuffer();
#else /******************else******************/
	sprite.pushSprite(0, 0);
#endif
}
