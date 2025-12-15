/**
 * 空项目: 361KB
 * wifi链接后: 291KB        (耗费 70kb)
 * 增加lvgl文字渲染: 266KB   (耗费 25kb)
 * 渲染jpg: 263KB
 * 渲染gif:
 */

// #include <esp-ai.h>
// // ESP_AI esp_ai;

#include <Arduino.h>
#include "eeui.h"
#include <WiFi.h>
#include <map>
#include <vector>

/**
 * 引入情绪
 * 快乐、伤心、愤怒、意外、专注、发愁、懊恼、困倦、疑问、恐惧、敬畏、肯定、否定
 * 建议gif图尺寸：160px * 160px  (建议不要超过100kb，否则c数组会很大)
 *
 * 必须使用 https://blog.csdn.net/weixin_43504224/article/details/130705038 方式渲染图片
 *
 * 图片颜色不对时，使用下面工具进行处理
 * 图片尺寸处理：https://ezgif.com/optimize/
 * 图片压缩处理：https://ezgif.com/optimize/
 * GIFu图制作：https://ezgif.com/maker
 */
#include "emos/wx_qrcode.h"
#include "emos/ap_qrcode.h"
#include "emos/wifi.h"
#include "emos/error.h"
#include "emos/listen.h"
#include "emos/sleep.h"
#include "emos/music.h"
#include "emos/tts_ing.h"
#include "emos/happy.h"
#include "emos/sad.h"
#include "emos/angry.h"
#include "emos/accident.h"
#include "emos/no.h"

// ESP-AI-V3 开发板
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define SCREEN_PAD_LEFT 8
#define SCREEN_PAD_RIGHT 8

// ESP-AI-V4 2寸屏幕/2.4寸屏幕
// #define SCREEN_WIDTH 320
// #define SCREEN_HEIGHT 240
// #define SCREEN_PAD_LEFT 8
// #define SCREEN_PAD_RIGHT 8

// 联域小暖 
// #define SCREEN_WIDTH 300
// #define SCREEN_HEIGHT 240
// #define SCREEN_PAD_LEFT 30
// #define SCREEN_PAD_RIGHT 5

TFT_eSPI tft = TFT_eSPI();
EEUI eeui;

/**
 * 定义一个情感与图片 URL 的映射数组
 */
const EEUIEmotionImagePair emotions[] = {
    // 状态动画
    {"联网中", &wifi_img},
    {"请配网", &wx_qrcode_img}, // 小程序配网时，会自动在屏幕上显示二维码，微信扫码即可跳转配网
    // {"请配网", &ap_qrcode_img}, // ap配网时，会自动在屏幕上显示二维码，微信扫码即可跳转配网
    {"发生错误", &error_img},
    {"聆听中", &listen_img},
    {"休息中", &sleep_img},
    {"唱歌中", &music_img},

    // 聊天表情动画(最好是说话的动作+表情)
    {"无情绪", &tts_ing_img}, // 这个表情必须配置
    {"快乐", &happy_img},
    {"伤心", &sad_img},
    {"愤怒", &angry_img},
    {"意外", &accident_img},
    {"否定", &no_img},

    // {"肯定", &kuai_le},
    // {"专注", &kuai_le},
    // {"发愁", &kuai_le},
    // {"懊恼", &kuai_le},
    // {"困倦", &kuai_le},
    // {"疑问", &kuai_le},
    // {"恐惧", &kuai_le},
    // {"敬畏", &kuai_le},
};

// String sentence = "";
void setup()
{
  Serial.begin(115200);

  delay(2000);
  // 初始化 屏幕
  tft.begin();
  tft.setRotation(3); // 设置屏幕方向 V4 开发板
  // tft.setRotation(1); // 设置屏幕方向  V3 开发板
  eeui.begin(&tft, emotions, sizeof(emotions) / sizeof(emotions[0]), SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_PAD_LEFT, SCREEN_PAD_RIGHT);
  // eeui.set_screen_circle();

  // eeui.render_gif_by_name("休息中");
  // vTaskDelay(500 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_ota_percent(50);

  // // test..
  // vTaskDelay(5000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_battery(89);
  // // eeui.render_battery(100);
  // eeui.recharge(true);
  // eeui.render_signal(2);
  // eeui.render_volume(1);
  // vTaskDelay(3000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_battery(100);

  /** OTA 检测升级中 **/
  // eeui.set_status_text("检测升级中", false, "bottom_center");
  // eeui.render_loading();
  // vTaskDelay(3000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.hide_loading();

  /** 联网中 **/
  // eeui.render_gif_by_name("联网中");
  // eeui.set_status_text("网络连接中", false, "bottom_center");

  /** 蓝牙配网二维码 **/
  // eeui.render_gif_by_name("请配网");
  // eeui.set_status_text("微信扫一扫配网", false, "bottom_center");

  /** AP配网二维码 **/
  // eeui.render_gif_by_name("请配网");
  // eeui.set_status_text("浏览器扫一扫配网", false, "bottom_center");

  /** 发生错误 **/
  // eeui.render_gif_by_name("发生错误");
  // eeui.set_status_text("抱歉，发生了错误！", false, "bottom_center");

  /** 表情-休息中 **/
  // eeui.render_gif_by_name("休息中");
  eeui.set_status_text("休息中", true, "");

  /** 聆听中 **/
  // eeui.render_gif_by_name("聆听中");
  // eeui.set_status_text("聆听中", true, "");

  /** 说话表情-唱歌中 **/
  // eeui.render_gif_by_name("唱歌中");
  // eeui.set_status_text("唱歌中", true, "");

  /** 说话表情-无情绪 **/
  // eeui.render_gif_by_name("说话中");
  // eeui.set_status_text("无情绪", true, "");

  /** 说话表情-快乐 **/
  // eeui.render_gif_by_name("快乐");
  // eeui.set_status_text("说话中", true, "");

  /** 说话表情-伤心 **/
  // eeui.render_gif_by_name("伤心");
  // eeui.set_status_text("说话中", true, "");

  /** 说话表情-愤怒 **/
  // eeui.render_gif_by_name("愤怒");
  // eeui.set_status_text("说话中", true, "");

  /** 说话表情-意外 **/
  // eeui.render_gif_by_name("意外");
  // eeui.set_status_text("说话中", true, "");

  /** 说话表情-否定 **/
  // eeui.render_gif_by_name("否定");
  // eeui.set_status_text("说话中", true, "");

  // 底部文字渲染
  // eeui.set_bottom_scrolling_text("我给你偷了好多好多的橘子，快来吃吧！");

  // 显示电量
  eeui.render_battery(100);
  // vTaskDelay(1500 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_battery(75);
  // vTaskDelay(1500 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_battery(50);
  // vTaskDelay(1500 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_battery(25);
  // vTaskDelay(1500 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_battery(0);
  // vTaskDelay(1500 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_battery(100);

  // 隐藏电量
  // eeui.hide_battery();
  // 设置为充电状态
  eeui.recharge(true);
  // vTaskDelay(5000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.recharge(true);
  // vTaskDelay(5000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.recharge(false);

  // 信号 1-4
  eeui.render_signal(3);
  // vTaskDelay(1000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_signal(2);
  // vTaskDelay(1000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_signal(1);
  // vTaskDelay(1000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_signal(0);
  // 信号链接动画
  // eeui.render_signal_conneact_ani(5);

  // 显示音量
  // eeui.render_volume(0.3);
  // vTaskDelay(1000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_volume(0.5);
  // vTaskDelay(3000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  eeui.render_volume(1);


  // 显示闹钟图标
  eeui.render_clock();
  // 隐藏闹钟图标
  // vTaskDelay(3000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.hide_clock();

  // OTA 升级
  // ing...
  // eeui.render_ota_percent(50);
  // int ota_progress = 0;
  // while (ota_progress < 100)
  // {
  //   eeui.render_ota_percent(ota_progress);
  //   ota_progress++;
  //   delay(100);
  // }

  // doro 对话
  eeui.set_bottom_scrolling_text("人，你知道吗？");
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  eeui.set_bottom_scrolling_text("我给你偷了好多好多的偶润结，快来吃吧！");
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  eeui.set_bottom_scrolling_text("doro 好开心呀！");

  // eeui.render_gif_by_name(emotions[0].image);
  // vTaskDelay(1000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.set_bottom_scrolling_text("人，你知道吗？");
  // vTaskDelay(1500 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_gif(emotions[1].image);
  // eeui.set_bottom_scrolling_text("我给你偷了好多好多的偶润结，快来吃吧！");
  // vTaskDelay(2000 / portTICK_PERIOD_MS); // 等待屏幕稳定
  // eeui.render_gif(emotions[2].image);
  // eeui.set_bottom_scrolling_text("doro 好开心呀！");
}

void loop()
{
  static unsigned long last_check = 0;
  if (millis() - last_check > 5000)
  {
    last_check = millis();
    Serial.printf("Free heap: %d KB\n", ESP.getFreeHeap() / 1024);
    // lv_mem_monitor_t mon;
    // lv_mem_monitor(&mon);
    // printf("LVGL memory: total_size: %d, free_size: %d, max_used: %d\n",
    //        (int)mon.total_size, (int)mon.free_size, (int)mon.max_used);
  }

  // eeui.render_gif_by_name("发生错误");
  // // eeui.set_bottom_scrolling_text("我给你偷了好多好多的橘子，快来吃吧！");
  // delay(3000);
  // eeui.render_gif_by_name("聆听中");
  // // eeui.set_bottom_scrolling_text("我给你偷了好多好多的橘子，快来吃吧！");
  // delay(3000);
  // eeui.render_gif_by_name("休息中");
  // // eeui.set_bottom_scrolling_text("我给你偷了好多好多的橘子，快来吃吧！");
  // delay(3000);

  // eeui.render_battery(89);
  // eeui.render_signal(2);
  // eeui.render_volume(1);
  // delay(3000);
  // eeui.render_battery(100);
  // eeui.render_signal(3);
  // eeui.render_volume(0.5);

  // eeui.set_status_text("说话中", true, "");
  // delay(300);
}

// ================================ TFT 测试 =====================================

// #include <Arduino.h>
// #include <TFT_eSPI.h>

// TFT_eSPI tft;

// void setup()
// {
//   Serial.begin(115200);
//   delay(1000);

//   tft.init();
//   tft.fillScreen(TFT_WHITE);
//   tft.setTextColor(TFT_BLACK);
//   tft.setCursor(10, 10);
//   tft.print("Hello, World!");
// }

// void loop() {}
