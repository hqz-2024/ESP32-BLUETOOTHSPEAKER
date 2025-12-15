/**
 * Copyright (c) 2024 小明IO
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
 * 请注意：将 ESP-AI 代码用于商业用途需要事先获得许可方的授权。
 * 删除与修改版权属于侵权行为，请尊重作者版权，避免产生不必要的纠纷。
 *
 * @author 小明IO
 * @email  1746809408@qq.com
 * @github https://github.com/wangzongming/esp-ai
 * @websit https://espai.fun
 */

#include "esp-ai-eyes.h"

int eyes_img_x = (SCREEN_W - IMG_W) / 2;
int eyes_img_y = (SCREEN_H - IMG_H) / 2;
int bi_yan_img_y = -SCREEN_H;
int xia_yan_pi_img_y = SCREEN_H - 50; // 下眼皮的y坐标
int shang_yan_pi_img_y = 0;           // 上眼皮的y坐标

struct LvglTaskProps
{
    bool *lvgl_initialized;
};
LvglTaskProps lvgl_task_props;

ESPAIEyes::ESPAIEyes()
{
}

void ESPAIEyes::begin(TFT_eSPI *tft)
{
    this->tft = tft;
    main_init();
    init_container();
    draw_yan_qiu();
}

void ESPAIEyes::my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    ESPAIEyes *ins = static_cast<ESPAIEyes *>(disp->user_data);
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;

    ins->tft->startWrite();
    ins->tft->setAddrWindow(area->x1, area->y1, w, h);

    // 分块写入，避免大数据阻塞
    uint32_t size = w * h;
    const uint32_t chunk = 512;
    while (size > 0)
    {
        uint32_t len = size > chunk ? chunk : size;
        ins->tft->pushColors((uint16_t *)color_p, len, true);
        size -= len;
        color_p += len;
    }

    // // 一次性写入
    // uint32_t size = w * h;
    // ins->tft->pushColors((uint16_t *)color_p, size, true);

    ins->tft->endWrite();
    lv_disp_flush_ready(disp);
}

void IRAM_ATTR lv_tick_isr(void *arg)
{
    // lv_tick_pending++;
    lv_tick_inc(1); // 每1ms加1
}
void lvgl_static_task(void *arg)
{
    LvglTaskProps *ctx = static_cast<LvglTaskProps *>(arg);
    if (!ctx)
    {
        Serial.println(F("[Error] lvgl_static_task ctx is null!"));
        vTaskDelete(NULL);
        return;
    }
    const TickType_t delay = pdMS_TO_TICKS(5); // 每5ms执行一次

    // 等待一小段时间，确保显示驱动完全就绪
    vTaskDelay(pdMS_TO_TICKS(200));
    // 标记LVGL初始化完成
    *(ctx->lvgl_initialized) = true;

    while (1)
    {
        // // 处理 tick
        // while (lv_tick_pending > 0)
        // {
        //   lv_tick_inc(1); // 安全地调用
        //   lv_tick_pending--;
        // }

        lv_timer_handler(); // LVGL 主循环处理函数
        vTaskDelay(delay);  // 延迟，避免占用CPU
    }
}
void ESPAIEyes::init_container()
{
    // 容器设置
    containe_bt = lv_obj_create(lv_scr_act());
    lv_obj_set_size(containe_bt, SCREEN_W, SCREEN_H);
    lv_obj_align(containe_bt, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    // lv_obj_center(containe_bt);
    // 设置为不可滚动、裁剪内容
    lv_obj_clear_flag(containe_bt, LV_OBJ_FLAG_SCROLLABLE);        // 禁止滚动
    lv_obj_set_scrollbar_mode(containe_bt, LV_SCROLLBAR_MODE_OFF); // 不显示滚动条
    lv_obj_set_style_clip_corner(containe_bt, true, 0);            // 启用裁剪（也可以省略这个）

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, SCREEN_W / 2);
    lv_style_set_border_width(&style, 0);
    // 设置容器背景为黑色并完全不透明，避免圆角抗锯齿与白色背景混合出现残影
    // lv_style_set_bg_color(&style, lv_color_black());
    // lv_style_set_bg_opa(&style, LV_OPA_COVER);

    lv_style_set_pad_all(&style, 0);
    lv_obj_add_style(containe_bt, &style, LV_PART_MAIN);
}

void ESPAIEyes::main_init()
{
    lv_init();
    // 保证 LVGL draw buffer 和根屏背景为黑，避免圆角抗锯齿与白色背景混合出现白边
    memset(buf, 0, sizeof(buf)); // 清零 draw buffer（0 => 黑）

#if defined(SCREEN_SIZE_240)
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 240);
#elif defined(SCREEN_SIZE_160)
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 100);
#endif

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_W;
    disp_drv.ver_res = SCREEN_H;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = this;
    lv_disp_drv_register(&disp_drv);

    // test...
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    lvgl_task_props = {
        .lvgl_initialized = &lvgl_initialized};

    xTaskCreateStaticPinnedToCore(
        lvgl_static_task,
        "LVGL Task",
        LVGL_TASK_STACK_SIZE, // 栈大小 word
        &lvgl_task_props,     // 参数
        1,                    // 优先级
        lvgl_task_stack,
        &lvgl_task_buf,
        1 // Core1 上运行
    );

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_isr,
        .name = "lv_tick_isr"};
    esp_timer_handle_t lv_tick_timer = NULL;
    esp_timer_create(&periodic_timer_args, &lv_tick_timer);
    esp_timer_start_periodic(lv_tick_timer, 1000);
}

// 定义动画回调
void anim_x_cb(void *var, int32_t v)
{
    eyes_img_x = v; // 更新全局变量
    lv_obj_set_x((lv_obj_t *)var, v);
}
void anim_size_cb(void *var, int32_t v)
{
    lv_obj_set_size((lv_obj_t *)var, v, v);
}

void eyelid_anim_y_cb(void *var, int32_t v)
{
    bi_yan_img_y = v; // 更新全局变量
    lv_obj_set_y((lv_obj_t *)var, v);
}

void ESPAIEyes::safe_canvas_draw_img(lv_obj_t *canvas, const lv_img_dsc_t *img, lv_coord_t x, lv_coord_t y)
{
    lv_draw_img_dsc_t dsc;
    lv_draw_img_dsc_init(&dsc);
    dsc.recolor_opa = LV_OPA_TRANSP; // 不重着色

    lv_canvas_draw_img(canvas, x, y, img, &dsc);
}

// 上下左右移动眼睛
void ESPAIEyes::move_eyes(const String &dir)
{
    // 获取当前 x 坐标 lv_obj_get_x_aligned
    // 左右移动动画
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, eyes_img); // 绑定动画对象

    if (dir == "left")
    {
        lv_anim_set_exec_cb(&a, anim_x_cb);
        lv_anim_set_values(&a, eyes_img_x, 0 - 10);
    }
    else if (dir == "center")
    {
        lv_anim_set_exec_cb(&a, anim_x_cb);
        lv_anim_set_values(&a, eyes_img_x, (SCREEN_W - IMG_W) / 2);
    }
    else if (dir == "right")
    {
        lv_anim_set_exec_cb(&a, anim_x_cb);
        lv_anim_set_values(&a, eyes_img_x, (SCREEN_W - IMG_W) + 10);
    }

    lv_anim_set_time(&a, 300);
    lv_anim_set_repeat_count(&a, 1);
    // lv_anim_set_playback_time(&a, 300); // 回程时间
    // lv_anim_set_playback_delay(&a, 100);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/**
 * 眨眼动画
 */
void ESPAIEyes::eyelid_ani(bool no_hide)
{
    // 先隐藏下眼皮
    if (xia_yan_pi_img)
    {
        xia_yan_pi_img_y = SCREEN_H;
        lv_obj_set_y(xia_yan_pi_img, xia_yan_pi_img_y);
    }

    // 初始化用于眨眼的眼皮
    if (!bi_yan_img)
    {
        bi_yan_img = lv_img_create(containe_bt);
#if defined(SCREEN_SIZE_240)
        lv_img_set_src(bi_yan_img, &eyes_240_yan_pi);
#elif defined(SCREEN_SIZE_160)
        lv_img_set_src(bi_yan_img, &eyes_160_yan_pi);
#endif
        lv_obj_set_y(bi_yan_img, -SCREEN_H); // 设置初始位置
        lv_obj_set_x(bi_yan_img, 0);         // 设置初始位置
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bi_yan_img); // 绑定动画对象
    lv_anim_set_exec_cb(&a, eyelid_anim_y_cb);
    lv_anim_set_repeat_count(&a, 1);
    lv_anim_set_time(&a, 300);
    if (bi_yan_img_y == 0 && !no_hide)
    {
        // 只需要睁眼即可
        lv_anim_set_values(&a, bi_yan_img_y, -SCREEN_H);
    }
    else
    {
        lv_anim_set_values(&a, bi_yan_img_y, 0);
        if (!no_hide)
        {
            lv_anim_set_playback_time(&a, 300); // 回程时间
            lv_anim_set_playback_delay(&a, 300);
        }
    }
    lv_anim_start(&a);
}

// 隐藏下眼皮动画
static void xia_yan_pi_img_ani(void *var, int32_t v)
{
    xia_yan_pi_img_y = v; // 更新全局变量
    lv_obj_set_y((lv_obj_t *)var, v);
}

/**
 * 绘制眼球
 */
void ESPAIEyes::draw_yan_qiu()
{
    eyes_img = lv_gif_create(containe_bt);

#if defined(SCREEN_SIZE_240)
    lv_gif_set_src(eyes_img, &eyes_200_red);
#elif defined(SCREEN_SIZE_160)
    lv_gif_set_src(eyes_img, &eyes_130_red);
#endif

    lv_obj_set_x(eyes_img, eyes_img_x); // 设置初始位置
    lv_obj_set_y(eyes_img, eyes_img_y); // 设置初始位置
}

/**
 * 绘制上眼皮
 * @param down_px 眼皮下移的像素数
 */
static lv_draw_img_dsc_t shang_yan_pi_dsc;
static lv_draw_rect_dsc_t shang_yan_pi_dsc2;
static lv_draw_arc_dsc_t shang_yan_pi_arc_dsc;
void ESPAIEyes::draw_shang_yan_pi()
{
    // 直接使用 png 图片
    shang_yan_pi_img = lv_img_create(containe_bt);

#if defined(SCREEN_SIZE_240)
    lv_img_set_src(shang_yan_pi_img, &eyes_240_xia_yan_pi);
#elif defined(SCREEN_SIZE_160)
    lv_img_set_src(shang_yan_pi_img, &eyes_160_xia_yan_pi);
#endif

    lv_obj_set_y(shang_yan_pi_img, -lv_obj_get_self_height(shang_yan_pi_img));
}

/**
 * 上眼皮出现和隐藏动画, 默认出来后1s隐藏
 * @param no_hide bool  是否不隐藏
 */
void ESPAIEyes::draw_shang_yan_pi_ani(bool no_hide)
{
    if (!shang_yan_pi_img)
    {
        draw_shang_yan_pi();
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, shang_yan_pi_img); // 绑定动画对象
    lv_anim_set_exec_cb(&a, eyelid_anim_y_cb);
    lv_anim_set_values(&a, -lv_obj_get_self_height(shang_yan_pi_img), 0);
    lv_anim_set_time(&a, 200);
    lv_anim_set_repeat_count(&a, 1);
    if (!no_hide)
    {
        lv_anim_set_playback_time(&a, 200); // 回程时间
        lv_anim_set_playback_delay(&a, 800);
    }
    // lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

/**
 * 绘制下眼皮
 */
void ESPAIEyes::draw_xia_yan_pi()
{
    xia_yan_pi_img_y = SCREEN_H - 50;
    xia_yan_pi_img = lv_img_create(containe_bt);
#if defined(SCREEN_SIZE_240)
    lv_img_set_src(xia_yan_pi_img, &eyes_240_yan_pi);
#elif defined(SCREEN_SIZE_160)
    lv_img_set_src(xia_yan_pi_img, &eyes_160_yan_pi);
#endif
    lv_obj_set_y(xia_yan_pi_img, xia_yan_pi_img_y);
}
/**
 * 下眼皮动画
 * @param no_hide  是否出现后不自动隐藏
 */
void ESPAIEyes::draw_xia_yan_pi_ani(bool no_hide)
{
    if (!xia_yan_pi_img)
    {
        draw_xia_yan_pi();
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, xia_yan_pi_img); // 绑定动画对象
    lv_anim_set_exec_cb(&a, xia_yan_pi_img_ani);
    lv_anim_set_values(&a, SCREEN_H, xia_yan_pi_img_y);
    lv_anim_set_time(&a, 200);
    lv_anim_set_repeat_count(&a, 1);
    if (!no_hide)
    {
        lv_anim_set_playback_time(&a, 200); // 回程时间
        lv_anim_set_playback_delay(&a, 800);
    }
    // lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}