/***************************************************
Copyright (c) 2020 Luis Llamas
(www.luisllamas.es)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License along with this program.  If not, see <http://www.gnu.org/licenses
****************************************************/

#ifndef _FACEEMOTIONS_h
#define _FACEEMOTIONS_h

#include <Arduino.h>

enum eEmotions {
	Normal = 0,        // 默认
	Angry,         // 愤怒的
	Glee,       // 快乐
	Happy,         // 快乐的
	Sad,           // 伤心
	Worried,    // 发愁的
	Focused,    // 专注的
	Annoyed,    // 恼怒的
	Surprised,     // 意外的
	Skeptic,    // 持怀疑态度
	Frustrated, // 懊恼的
	Unimpressed,// 不感兴趣
	Sleepy,     // 困倦的
	Suspicious, // 感觉可疑的，
	Squint,        // 眯眼看
	Furious,    // 狂怒的 热烈兴
	Scared,     // 惊恐的，恐惧
	Awe,        // 敬畏
	EMOTIONS_COUNT
};

#endif
