/*
 * MIT License
 *
 * Copyright (c) 2025-至今 小明IO
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @author 小明IO
 * @email  1746809408@qq.com
 * @github https://github.com/wangzongming/esp-ai
 * @websit https://espai.fun
 */

#pragma once
// #define SCREEN_SIZE_240 // 使用 240 * 240 尺寸的圆形屏幕
#define SCREEN_SIZE_160 // 使用 160 * 160  尺寸的圆形屏幕

#include <lvgl.h>
#include <TFT_eSPI.h>

// 包含眼睛图像
#if defined(SCREEN_SIZE_240)
#include "imgs/eyes_200_red.h"
#include "imgs/eyes_240_yan_pi.h"
#include "imgs/eyes_240_xia_yan_pi.h"
#elif defined(SCREEN_SIZE_160)
#include "imgs/eyes_130_red.h"
#include "imgs/eyes_160_yan_pi.h"
#include "imgs/eyes_160_xia_yan_pi.h"
#endif

// lvgl 任务堆栈大小（单位：字）
#define LVGL_TASK_STACK_SIZE 4096
#define LVGL_TASK_PRIORITY 1

// 屏幕和图片大小
#if defined(SCREEN_SIZE_160)
#define SCREEN_W 160
#define SCREEN_H 160
#define IMG_W 130
#define IMG_H 130
#endif
#if defined(SCREEN_SIZE_240)
#define SCREEN_W 240
#define SCREEN_H 240
#define IMG_W 200
#define IMG_H 200
#endif

class ESPAIEyes
{
public:
    ESPAIEyes();
    void begin(TFT_eSPI *tft);

    /**
     * 渲染眼球
     */
    void draw_yan_qiu();

    /**
     * 绘制上眼皮, 会自动调用 draw_shang_yan_pi_ani 进行眼皮绘制.
     * @param no_hide 出现后不隐藏
     */
    void draw_shang_yan_pi_ani(bool no_hide);

    /**
     * 绘制下眼皮, 会自动调用 draw_xia_yan_pi 进行眼皮绘制.
     * @param no_hide 出现后不隐藏
     */
    void draw_xia_yan_pi_ani(bool no_hide);

    /**
     * 上下左右移动眼睛
     */
    void move_eyes(const String &dir);

    /**
     * 眨眼动画
     * @param no_hide 出现后不隐藏
     */
    void eyelid_ani(bool no_hide);

private:
    TFT_eSPI *tft;
    lv_disp_draw_buf_t draw_buf;

#if defined(ARDUINO_ESP32S3_DEV)
#if defined(SCREEN_SIZE_240)
    lv_color_t *buf = (lv_color_t *)heap_caps_malloc(LV_HOR_RES_MAX * 240 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
#elif defined(SCREEN_SIZE_160)
    lv_color_t *buf = (lv_color_t *)heap_caps_malloc(LV_HOR_RES_MAX * 100 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
#endif

#else

#if defined(SCREEN_SIZE_240)
    lv_color_t buf[LV_HOR_RES_MAX * 240];
#elif defined(SCREEN_SIZE_160)
    lv_color_t buf[LV_HOR_RES_MAX * 100];
#endif

#endif

    StaticTask_t lvgl_task_buf;
    StackType_t lvgl_task_stack[LVGL_TASK_STACK_SIZE];

    lv_obj_t *containe_bt = nullptr;
    bool lvgl_initialized = false;

    lv_obj_t *eyes_img = nullptr;

    // 闭眼的眼皮
    lv_obj_t *bi_yan_img = nullptr;

    // 下眼皮
    lv_obj_t *xia_yan_pi_img = nullptr;

    // 上眼皮
    lv_obj_t *shang_yan_pi_img = nullptr;

    lv_obj_t *yan_pi_canvas = nullptr;
#if defined(ARDUINO_ESP32S3_DEV)
    uint8_t *cbuf = (uint8_t *)heap_caps_malloc(SCREEN_W * SCREEN_H * 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    uint8_t cbuf[SCREEN_W * SCREEN_H * 4]; // RGBA 缓冲区（4 字节/像素）
#endif

    void init_container();
    static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    void main_init();

    void safe_canvas_draw_img(lv_obj_t *canvas, const lv_img_dsc_t *img, lv_coord_t x, lv_coord_t y);

    /**
     * 绘制上眼皮
     */
    void draw_shang_yan_pi();
    /**
     * 绘制下眼皮
     */
    void draw_xia_yan_pi();
};
