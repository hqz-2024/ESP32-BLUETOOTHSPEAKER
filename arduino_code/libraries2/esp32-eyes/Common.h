#ifndef COMMON_h
#define COMMON_h

// =======================================
// =======================================
// =======================================
// 定义使用的屏幕类型
// #define SCREEN_TYPE 0      // 0: TFT   
#define SCREEN_TYPE 1   // 1: OLED 
// =======================================
// =======================================
// =======================================






#if SCREEN_TYPE == 1

#include <U8g2lib.h>
// OLED基类指针
extern U8G2 *u8g2;

#else/**************************else*******************************/

#include "SPI.h"
#include "TFT_eSPI.h"
#include "U8g2_for_TFT_eSPI.h"
// TFT基类指针
extern U8g2_for_TFT_eSPI *u8g2;       // U8g2 font instance

#endif
#endif