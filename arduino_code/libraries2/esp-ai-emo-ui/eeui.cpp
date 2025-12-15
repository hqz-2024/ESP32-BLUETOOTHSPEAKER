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

#include "eeui.h"

portMUX_TYPE EEUI::timerMux = portMUX_INITIALIZER_UNLOCKED;
// 静态任务堆栈大小（单位：字）
#define LVGL_TASK_STACK_SIZE 4096
#define LVGL_TASK_PRIORITY 1
// 静态任务资源
static StaticTask_t lvgl_task_buf;
static StackType_t lvgl_task_stack[LVGL_TASK_STACK_SIZE];
struct LvglTaskProps
{
    bool *lvgl_initialized;
};
LvglTaskProps lvgl_task_props;

volatile uint32_t lv_tick_pending = 0;
static TaskHandle_t lvgl_task_handle = NULL;

// 在任何任务 / 回调中调用：
void safe_delete_obj(lv_obj_t *obj)
{
    if (!obj)
        return;
    // 将删除操作排到 LVGL 线程上下文执行，避免竞态
    lv_async_call([](void *p)
                  {
        lv_obj_t *o = (lv_obj_t*)p;
        if(!lv_obj_is_valid(o)) return;
        // 先断开 img 的数据源（停止 GIF）
        lv_img_set_src(o, NULL);
        // 再删除对象
        lv_obj_del(o); }, obj);
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
        // 处理 tick
        while (lv_tick_pending > 0)
        {
            lv_tick_inc(5); // 安全地调用
            lv_tick_pending--;
        }

        lv_timer_handler(); // LVGL 主循环处理函数
        vTaskDelay(delay);  // 延迟，避免占用CPU
    }
}

static bool botom_label_hide_ani_ing = false;
static bool botom_label_show_ani_ing = false;

static bool gif_hide_ani_ing = false;
static SemaphoreHandle_t gif_show_ani_ing;
static SemaphoreHandle_t gif_render_ing;
static SemaphoreHandle_t label_mutex;

// 简洁模式
static bool is_set_pure_mode = false;
// 背景颜色
static lv_color_t eeui_bg_color = lv_color_white();

struct RenderGifTOParam
{
    EEUI *self;
    const lv_img_dsc_t *image;
    bool need_ani;
    lv_coord_t x_ofs; // 新增字段
    lv_coord_t y_ofs; // 新增字段
};

EEUI::EEUI()
{
}

void IRAM_ATTR lv_tick_isr(void *arg)
{
    lv_tick_pending++;
}

void EEUI::begin(TFT_eSPI *tft, const EEUIEmotionImagePair *emotions, size_t emotion_count, int screen_width = 240, int screen_height = 240, int screen_pad_left = 0, int screen_pad_right = 0)
{
    label_mutex = xSemaphoreCreateBinary();
    xSemaphoreGive(label_mutex);

    gif_show_ani_ing = xSemaphoreCreateBinary();
    xSemaphoreGive(gif_show_ani_ing);

    gif_render_ing = xSemaphoreCreateBinary();
    xSemaphoreGive(gif_render_ing);

    this->screen_width = screen_width;
    this->screen_height = screen_height;
    this->tft = tft;
    this->emotions = emotions;
    this->emotion_count = emotion_count;
    this->screen_pad_left = screen_pad_left;
    this->screen_pad_right = screen_pad_right;

    tft->setSwapBytes(false); // optional: for tft screen itself if needed

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = this->screen_width;
    disp_drv.ver_res = this->screen_height;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = this; // 把 EEUI 对象传入
    lv_disp_drv_register(&disp_drv);

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
    lvgl_initialized = true;

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_isr,
        .name = "lv_tick_isr"};
    esp_timer_handle_t lv_tick_timer = NULL;
    esp_timer_create(&periodic_timer_args, &lv_tick_timer);
    // 1000 us = 1 ms
    esp_timer_start_periodic(lv_tick_timer, 5000);

    init_container();
}

void EEUI::await_lvgl_initialized()
{
    // 等待LVGL初始化完成
    while (!lvgl_initialized)
    {
        Serial.println("等待LVGL初始化完成...");
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void EEUI::my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    EEUI *ui = static_cast<EEUI *>(disp->user_data);
    ui->tft->startWrite();
    ui->tft->setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
    ui->tft->pushColors((uint16_t *)&color_p->full, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1), true);
    ui->tft->endWrite();
    lv_disp_flush_ready(disp);
}

void EEUI::set_eeui_bg_color(const lv_color_t color)
{
    eeui_bg_color = color;
}

/**
 * 容器设置
 */
void EEUI::init_container()
{
    // 容器设置
    containe_bt = lv_obj_create(lv_scr_act());
    lv_obj_set_size(containe_bt, screen_width, screen_height);
    lv_obj_align(containe_bt, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    // 设置为不可滚动、裁剪内容
    lv_obj_clear_flag(containe_bt, LV_OBJ_FLAG_SCROLLABLE);        // 禁止滚动
    lv_obj_set_scrollbar_mode(containe_bt, LV_SCROLLBAR_MODE_OFF); // 不显示滚动条
    lv_obj_set_style_clip_corner(containe_bt, true, 0);            // 启用裁剪（也可以省略这个）

    static lv_style_t eeui_container_style;
    lv_style_init(&eeui_container_style);
    lv_style_set_radius(&eeui_container_style, 0);
    lv_style_set_pad_left(&eeui_container_style, screen_pad_left);
    lv_style_set_pad_right(&eeui_container_style, screen_pad_right);

    // 背景色设置
    lv_style_set_bg_color(&eeui_container_style, eeui_bg_color); // 使用 lv_color_make(R,G,B)
    lv_style_set_bg_opa(&eeui_container_style, LV_OPA_COVER);    // 不透明

    lv_obj_add_style(containe_bt, &eeui_container_style, LV_PART_MAIN);
}

/**
 * 底部文字动画
 */

static void anim_x_cb(void *var, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)var, v);
}
static void anim_y_cb(void *var, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)var, v);
}

static void botom_label_hide_anim_finished_cb(lv_anim_t *a)
{
    botom_label_hide_ani_ing = false;
}
static void botom_label_show_anim_finished_cb(lv_anim_t *a)
{
    botom_label_show_ani_ing = false;
    // 文字显示完毕就算渲染完毕
    // botom_label_render_ing = false;
    // 动画渲染完毕，释放锁
    xSemaphoreGive(label_mutex);
}

void EEUI::hide_text_left(lv_obj_t *label)
{
    botom_label_hide_ani_ing = true;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_coord_t pad_left = lv_obj_get_style_pad_left(containe_bt, 0);
    lv_anim_set_values(&a, lv_obj_get_x(label), 0 - ((lv_obj_get_width(label) + get_left_right_padding())));
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_ready_cb(&a, botom_label_hide_anim_finished_cb);
    lv_anim_start(&a);
}

/**
 * 滑动出现文本
 */
void EEUI::show_bottom_text(lv_obj_t *label)
{
    if (label == NULL)
    {
        xSemaphoreGive(label_mutex);
        return;
    }

    // 停止所有关联的定时器（防止隐藏动画干扰）
    if (hide_timer != NULL)
    {
        lv_timer_del(hide_timer);
        hide_timer = NULL;
    }
    lv_anim_del(label, NULL);

    if (label == nullptr)
    {
        xSemaphoreGive(label_mutex);
        Serial.println("错误：img 是空指针！");
        return;
    }

    botom_label_show_ani_ing = true;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_values(&a, screen_height, screen_height - lv_obj_get_height(botom_scroll_label) - get_top_bottom_padding());
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, anim_y_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_bounce);
    lv_anim_set_ready_cb(&a, botom_label_show_anim_finished_cb);
    lv_anim_start(&a);
}

struct ShowTextProps
{
    EEUI *self;
    lv_obj_t *data;
};
struct ScrollTextParam
{
    EEUI *self;
    const char *text;
    bool need_ani;
    const char *align;
};

// 创建包含位置信息的参数结构体
    struct PositionTextParam {
        EEUI *self;
        const char *text;
        bool need_ani;
        const char *align;
        lv_coord_t x;
        lv_coord_t y;
    };

void EEUI::set_bottom_scrolling_text_todo(const char *text)
{

    // 让上一个动画完毕
    if (hide_timer != NULL && lv_obj_is_valid(botom_scroll_label))
    {

        lv_timer_del(hide_timer);
        hide_timer = NULL;

        hide_text_left(botom_scroll_label);

        auto *param = new ScrollTextParam{this, text};
        lv_timer_create(
            [](lv_timer_t *t)
            {
                ScrollTextParam *props = static_cast<ScrollTextParam *>(t->user_data);
                EEUI *self = props->self;
                const char *text = props->text;

                self->set_bottom_scrolling_text_todo(text);
                delete props; // 改用 delete
                lv_timer_del(t);
            },
            400,
            (void *)param);
        return;
    }

    if (botom_scroll_label == NULL)
    {
        Serial.println("创建底部滚动文字标签");
        botom_scroll_label = lv_label_create(containe_bt);
        lv_label_set_text(botom_scroll_label, "");
        lv_label_set_long_mode(botom_scroll_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(botom_scroll_label, screen_width - get_left_right_padding());
        lv_obj_set_style_text_font(botom_scroll_label, &font_chinese_16, 0);
        // 让文字保持最上层显示
        lv_obj_move_foreground(botom_scroll_label);

        lv_style_init(&botom_scroll_label_style);
        lv_style_set_text_color(&botom_scroll_label_style, theme_color);
        lv_obj_add_style(botom_scroll_label, &botom_scroll_label_style, LV_PART_MAIN);
    }

    lv_label_set_text(botom_scroll_label, text);
    // 强制刷新布局（同步计算尺寸）
    if (botom_scroll_label && lv_obj_is_valid(botom_scroll_label))
    {
        lv_obj_update_layout(botom_scroll_label);
    }
    // botom_label_render_ing = false;

    // 隐藏文本
    lv_obj_set_pos(botom_scroll_label, 0, screen_height);

    // 文本出现动画
    show_bottom_text(botom_scroll_label);

    // 文本消失动画
    auto *param = new ShowTextProps{this, botom_scroll_label};
    hide_timer = lv_timer_create(
        [](lv_timer_t *timer)
        {
            ShowTextProps *props = static_cast<ShowTextProps *>(timer->user_data);
            EEUI *self = props->self;
            lv_obj_t *data = props->data;
            self->hide_text_left(data);
            delete props;
            lv_timer_del(timer);
            self->hide_timer = NULL;
        },
        3000, // 间隔时间
        param // 传递当前对象作为用户数据
    );
}

/**
 * 设置底部滚动文字， 后调用的会直接覆盖之前的文字
 * @param text 要显示的文字内容
 */
void EEUI::set_bottom_scrolling_text(const char *text)
{
    if (is_set_pure_mode)
        return;

    // 等待上一个动画结束（阻塞直到拿到锁）
    xSemaphoreTake(label_mutex, portMAX_DELAY);

    // 等待LVGL初始化完成
    await_lvgl_initialized();

    auto *param = new ScrollTextParam{this, text}; // 记得释放
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<ScrollTextParam *>(p);
                      param->self->set_bottom_scrolling_text_todo(param->text);
                      delete param; // 释放内存
                  },
                  param);
}

/**
 * 滑动出现状态文本
 */
void EEUI::show_status_text(lv_obj_t *label)
{
    if (label == NULL)
        return;

    lv_anim_del(label, NULL);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_values(&a, -35, -5);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, anim_y_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_bounce);
    lv_anim_start(&a);
}

void EEUI::set_status_text_todo(const char *text, bool need_ani, const char *align)
{
    if (status_scroll_label == NULL)
    {
        status_scroll_label = lv_label_create(containe_bt);
        lv_label_set_text(status_scroll_label, "");
        lv_label_set_long_mode(status_scroll_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(status_scroll_label, screen_width - get_left_right_padding());
        lv_obj_set_style_text_font(status_scroll_label, &font_chinese_16, 0);
        // 让文字保持最上层显示
        lv_obj_move_foreground(status_scroll_label);

        lv_style_init(&status_scroll_label_style);
        lv_style_set_text_color(&status_scroll_label_style, gray_color2);
        lv_obj_add_style(status_scroll_label, &status_scroll_label_style, LV_PART_MAIN);
    }
    else
    {
        if (need_ani)
        {
            // 隐藏当前文字
            hide_text_left(status_scroll_label);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    lv_label_set_text(status_scroll_label, text);
    // 强制刷新布局（同步计算尺寸）
    if (status_scroll_label && lv_obj_is_valid(status_scroll_label))
    {
        lv_obj_update_layout(status_scroll_label);
    }

    if (need_ani)
    {

        // if (is_circle_screen)
        // {
        //     lv_obj_set_pos(status_scroll_label, screen_width / 2 - lv_obj_get_self_width(status_scroll_label) / 2 - get_top_bottom_padding() / 2, screen_height - get_top_bottom_padding() + 3 - lv_obj_get_self_height(status_scroll_label));
        // }
        // else
        // {
        // 隐藏文本
        lv_obj_set_pos(status_scroll_label, 0, -35);
        // 文本出现动画
        show_status_text(status_scroll_label);
        // }
    }

    if (strcmp(align, "bottom_center") == 0)
    {
        lv_obj_set_pos(status_scroll_label, screen_width / 2 - lv_obj_get_self_width(status_scroll_label) / 2 - get_top_bottom_padding() / 2, screen_height - get_top_bottom_padding() + 3 - lv_obj_get_self_height(status_scroll_label));
    }

    xSemaphoreGive(label_mutex);

    // 文本前面的小圆点
    // ing...
}

/**
 * 设置状态文本并指定显示位置
 * @param text 要显示的文本内容
 * @param need_ani 是否需要动画效果
 * @param x 文本的X坐标位置
 * @param y 文本的Y坐标位置
 */
void EEUI::set_status_text_with_position(const char *text, bool need_ani,const char *align, lv_coord_t x, lv_coord_t y)
{
     // 定义状态文字滚动标签
    lv_obj_t *status_scroll_label = nullptr;
    // 如果标签对象不存在，则创建它
    Serial.println("[EEUI] set_status_text_with_position: 开始处理文本显示");
    if (status_scroll_label == NULL)
    {
        status_scroll_label = lv_label_create(containe_bt);
        lv_label_set_text(status_scroll_label, "");
        lv_label_set_long_mode(status_scroll_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(status_scroll_label, screen_width - get_left_right_padding());
        lv_obj_set_style_text_font(status_scroll_label, &font_chinese_16, 0);
        // 让文字保持最上层显示
        lv_obj_move_foreground(status_scroll_label);

        lv_style_init(&status_scroll_label_style);
        lv_style_set_text_color(&status_scroll_label_style, gray_color2);
        lv_obj_add_style(status_scroll_label, &status_scroll_label_style, LV_PART_MAIN);
    }
    else
    {
        if (need_ani)
        {
            // 隐藏当前文字
            hide_text_left(status_scroll_label);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    // 设置文本内容
    Serial.printf("[EEUI] set_status_text_with_position: 设置文本内容: %s\n", text);
    lv_label_set_text(status_scroll_label, text);
    
    // 强制刷新布局（同步计算尺寸）
    if (status_scroll_label && lv_obj_is_valid(status_scroll_label))
    {
        lv_obj_update_layout(status_scroll_label);
    }

    if (need_ani)
    {
        Serial.println("[EEUI] set_status_text_with_position: 执行显示动画");
        // 隐藏文本准备动画
        lv_obj_set_pos(status_scroll_label, 0, -35);
        // 文本出现动画
        show_status_text(status_scroll_label);
        // 动画完成后设置到指定位置
        vTaskDelay(pdMS_TO_TICKS(500)); // 等待动画完成
        Serial.println("[EEUI] set_status_text_with_position: 显示动画完成");
    }

    // 设置文本到指定的X,Y坐标位置
    if (strcmp(align, "bottom_center") == 0)
    {
        lv_obj_set_pos(status_scroll_label, screen_width / 2 - lv_obj_get_self_width(status_scroll_label) / 2 - get_top_bottom_padding() / 2 + x*lv_obj_get_self_width(status_scroll_label), screen_height - get_top_bottom_padding() + 3 - lv_obj_get_self_height(status_scroll_label) + y*lv_obj_get_self_height(status_scroll_label));
    }

    // 释放互斥锁
    Serial.println("[EEUI] set_status_text_with_position: 释放标签互斥锁，操作完成");
    xSemaphoreGive(label_mutex);
}

/**
 * 设置设备状态文字，后调用的会直接覆盖之前的文字
 * @param text 要显示的文字内容
 * @param need_ani 是否需要出场动画效果
 * @param align 对齐方式  "top_left" | "bottom_center"
 */
void EEUI::set_status_text(const char *text, bool need_ani = true, const char *align = "top_left")
{
    if (is_set_pure_mode)
        return;

    xSemaphoreTake(label_mutex, portMAX_DELAY);
    // 等待LVGL初始化完成
    await_lvgl_initialized();

    auto *param = new ScrollTextParam{this, text, need_ani, align}; // 记得释放
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<ScrollTextParam *>(p);
                      param->self->set_status_text_todo(param->text, param->need_ani, param->align);
                      delete param; // 释放内存
                  },
                  param);
}

/**
 * 在指定位置显示状态文本
 * @param text 要显示的文本内容
 * @param need_ani 是否需要动画效果
 * @param x 文本的X坐标位置
 * @param y 文本的Y坐标位置
 */
void EEUI::set_status_text_positioned(const char *text, bool need_ani,const char *align,lv_coord_t x, lv_coord_t y)
{
    if (is_set_pure_mode)
        return;

    xSemaphoreTake(label_mutex, portMAX_DELAY);
    // 等待LVGL初始化完成
    await_lvgl_initialized();
    
    auto *param = new PositionTextParam{this, text, need_ani, align, x, y}; // 记得释放
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<PositionTextParam *>(p);
                      Serial.println("set_status_text_positioned");
                      param->self->set_status_text_with_position(param->text, param->need_ani, param->align, param->x, param->y);
                    delete param; // 释放内存
                  },
                  param);
}

static void gif_hide_anim_finished_cb(lv_anim_t *a)
{
    gif_hide_ani_ing = false;
}
static void gif_show_anim_finished_cb(lv_anim_t *a)
{
    // gif_show_ani_ing = false;
    // test...
    xSemaphoreGive(gif_show_ani_ing);

    // 显示完毕就算渲染完毕
    // test...
    // gif_render_ing = false;
    // xSemaphoreGive(gif_render_ing);
}

struct GifTodoParam
{
    EEUI *self;
    const lv_img_dsc_t *image;
    bool need_ani;
    lv_obj_t *emo_img;
};
/**
 * 渲染 GIF 图片
 * @param image GIF 图片数据
 * @param need_ani 需要执行出场动画
 */
void EEUI::render_gif_todo(const lv_img_dsc_t *image, bool need_ani = false,lv_coord_t x_ofs = 0, lv_coord_t y_ofs = 0)
{

    // 如果已存在，先删除旧的
    if (emo_img != NULL)
    {
        if (emo_img && lv_obj_is_valid(emo_img))
        {
            // 1) 先停掉挂在该对象上的动画/过渡，避免动画回调里再次访问
            lv_anim_del(emo_img, nullptr);
            // 2) 先把成员指针置空，防止并发/重入再次使用同一指针
            lv_obj_t *to_del = emo_img;
            emo_img = nullptr;
            // 3) 用延迟删除，等 LVGL 把当前回调/迭代结束后再真正释放
            lv_obj_del_delayed(to_del, 100);
        }
        emo_img = NULL;
    }
    
    emo_img = lv_gif_create(containe_bt);
    lv_obj_align(emo_img, LV_ALIGN_CENTER, x_ofs, y_ofs);
    if (need_ani)
    {
        lv_obj_set_x(emo_img, screen_width);
        show_status_gif(emo_img);
    }
    else
    {
        xSemaphoreGive(gif_show_ani_ing);
    }
    if (!emo_img)
    {
        xSemaphoreGive(gif_render_ing);
        return;
    }
    lv_gif_set_src(emo_img, image);
    move_text_top();
    xSemaphoreGive(gif_render_ing);
}

/**
 * 让文字保持在最上层显示
 */
void EEUI::move_text_top()
{
    if (botom_scroll_label)
        lv_obj_move_foreground(botom_scroll_label);
    if (status_scroll_label)
        lv_obj_move_foreground(status_scroll_label);
    if (volume_bar)
        lv_obj_move_foreground(volume_bar);
    if (volume_bar_label)
        lv_obj_move_foreground(volume_bar_label);
}

void EEUI::render_gif(const lv_img_dsc_t *image, bool need_ani,lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    // 等待LVGL初始化完成
    await_lvgl_initialized();
    xSemaphoreTake(gif_render_ing, portMAX_DELAY);

    auto *param = new RenderGifTOParam{this, image, need_ani, x_ofs, y_ofs}; // 记得释放
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<RenderGifTOParam *>(p);
                      param->self->render_gif_todo(param->image, param->need_ani, param->x_ofs, param->y_ofs);  
                      delete param; // 释放内存
                  },
                  param);
}

// 渲染传入的表情Map中的某个Gif表情
void EEUI::render_gif_by_name(const char *image_name,lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    // 等待LVGL初始化完成
    await_lvgl_initialized();

    // 查找对应的表情
    for (size_t i = 0; i < emotion_count; ++i)
    {
        if (emotions[i].emotion_name == image_name)
        {
            render_gif(emotions[i].image, false, x_ofs, y_ofs);
            return;
        }
    }
}

struct RenderBatParam
{
    EEUI *self;
    int percent;
};

/**
 * 绘制电量图标
 * 四格电量的样式  ing...
 *
 * @param percent Int 电量百分比 0 - 100
 */
void EEUI::render_battery_todo(int percent)
{
    if (is_set_pure_mode)
        return;

    // 电池图标尺寸
    const int width = bat_width;
    const int height = bat_height;
    const int padding = 2;
    const int header_width = 4;                                             // 电池头宽度                            // 内边距
    const int segment_width = (width - header_width - padding * 2 - 3) / 4; // 每格电量的宽度

    // 如果 canvas 不存在则创建
    if (battery_canvas == NULL)
    {
        battery_cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height));
        battery_canvas = lv_canvas_create(lv_scr_act());
        lv_canvas_set_buffer(battery_canvas, battery_cbuf, width, height, LV_IMG_CF_TRUE_COLOR);
    }

    // 计算位置
    int x = get_bat_x();
    int y = get_bat_y();
    lv_obj_set_pos(battery_canvas, x, y);

    // 清空画布
    lv_canvas_fill_bg(battery_canvas, lv_color_white(), LV_OPA_TRANSP);

    // 绘制电池外框
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.border_width = 1;
    rect_dsc.border_color = icon_color;
    rect_dsc.bg_color = lv_color_white();
    rect_dsc.radius = 2;
    lv_canvas_draw_rect(battery_canvas, 0, 0, width - 1 - header_width, height, &rect_dsc);

    // 绘制电池头
    rect_dsc.radius = 0;
    lv_canvas_draw_rect(battery_canvas, width - header_width - 1, height / 4 + 1, 2, height / 2, &rect_dsc);

    // 计算当前电量格数(0-4格)
    int bars = (percent * 4) / 100;
    if (percent > 0 && bars == 0)
        bars = 1; // 只要有电就至少显示1格

    // 绘制电量格
    rect_dsc.border_width = 0;
    rect_dsc.bg_color = percent <= 25 ? lv_color_make(255, 0, 0) : icon_color; // 低电量显示红色

    for (int i = 0; i < bars; i++)
    {
        lv_canvas_draw_rect(battery_canvas,
                            padding + i * (segment_width + 1), // x
                            padding,                           // y
                            segment_width,                     // width
                            height - padding * 2,              // height
                            &rect_dsc);
    }

    // 在电池画布上绘制充电图标
    if (is_recharge)
    {
        lv_draw_label_dsc_t label_dsc;
        lv_draw_label_dsc_init(&label_dsc);
        label_dsc.color = grad_bg_color1;
        label_dsc.font = &lv_font_montserrat_10;
        lv_canvas_draw_text(battery_canvas, bat_width / 2 - 6, 2, 18, &label_dsc, LV_SYMBOL_CHARGE);
    }
    xSemaphoreGive(label_mutex);
}

void EEUI::render_battery(int percent)
{
    if (is_set_pure_mode)
        return;

    xSemaphoreTake(label_mutex, portMAX_DELAY);
    // 等待LVGL初始化完成
    await_lvgl_initialized();
    bat_percent = percent;

    auto *param = new RenderBatParam{this, percent}; // 记得释放
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<RenderBatParam *>(p);
                      param->self->render_battery_todo(param->percent);
                      delete param; // 释放内存
                  },
                  param);

    // 防止渲染电量和充电中同时调用，这时候充电中图标会定位错误，所以增加 延时。
    delay(100);
}

void EEUI::hide_battery()
{
    if (is_set_pure_mode)
        return;

    // 等待LVGL初始化完成
    await_lvgl_initialized();

    if (battery_canvas)
    {
        // lv_obj_del(battery_canvas);
        safe_delete_obj(emo_img);
        battery_canvas = nullptr;
    }
}

// 向左滑动出现图片
void EEUI::show_status_gif(lv_obj_t *img)
{
    lv_obj_t *_img = img ? img : emo_img;
    if (_img == nullptr)
    {
        Serial.println("错误：img 是空指针！");
        return;
    }

    // gif_show_ani_ing = true;
    xSemaphoreTake(gif_show_ani_ing, portMAX_DELAY);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _img);
    lv_anim_set_values(&a, screen_width, 0);
    lv_anim_set_time(&a, 500);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_bounce);
    lv_anim_set_ready_cb(&a, gif_show_anim_finished_cb);
    lv_anim_start(&a);
}

// 向左滑动隐藏图片
void EEUI::hide_status_gif(lv_obj_t *img)
{
    lv_obj_t *_img = img ? img : emo_img;
    if (_img == nullptr)
    {
        Serial.println("错误：img 是空指针！");
        return;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, _img);
    if (!_img || !lv_obj_is_valid(_img))
        return; // 安全检查

    gif_hide_ani_ing = true;
    lv_anim_set_values(&a, lv_obj_get_x(_img), 0 - lv_obj_get_width(_img) / 2 - screen_width / 2);
    lv_anim_set_time(&a, 500);
    lv_anim_set_exec_cb(&a, anim_x_cb);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_ready_cb(&a, gif_hide_anim_finished_cb);
    lv_anim_start(&a);
}

// 设置是否在充电中
void EEUI::recharge(bool _is_recharge)
{
    if (is_set_pure_mode)
        return;
    await_lvgl_initialized();
    if (is_recharge != _is_recharge)
    {
        is_recharge = _is_recharge;
        render_battery(bat_percent);
    }

    // 充电特效可能需要去除
    // render_gif(&rechargeing_gif, true);
    // // 渲染 gif
    // auto *param = new ShowTextProps{this, emo_img};
    // hide_status_gif_timer = lv_timer_create(
    //     [](lv_timer_t *timer)
    //     {
    //         ShowTextProps *props = static_cast<ShowTextProps *>(timer->user_data);
    //         EEUI *self = props->self;
    //         lv_obj_t *data = props->data;
    //         self->hide_status_gif(data);
    //         delete props;
    //         lv_timer_del(timer);
    //         self->hide_status_gif_timer = NULL;
    //     },
    //     2000,
    //     param);
}

int EEUI::get_left_right_padding()
{
    return lv_obj_get_style_pad_left(containe_bt, 0) + lv_obj_get_style_pad_right(containe_bt, 0);
}
int EEUI::get_top_bottom_padding()
{
    return lv_obj_get_style_pad_top(containe_bt, 0) + lv_obj_get_style_pad_bottom(containe_bt, 0);
}
void EEUI::set_theme_color(const lv_color_t color)
{
    theme_color = color;
    icon_color = color;
    gray_color2 = color;
}
void EEUI::set_pure_mode()
{
    is_set_pure_mode = true;
}

void EEUI::render_signal_todo(int strength)
{
    signal = strength;
    int width = signal_width;
    int height = signal_height;
    // 清空 canvas
    lv_canvas_fill_bg(signal_canvas, lv_color_make(0xFF, 0xFF, 0xFF), LV_OPA_COVER);

    // 用 arc 模拟 Wi-Fi 信号
    // 有信号的时候
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = icon_color;
    arc_dsc.width = 2;

    // 没信号的时候
    lv_draw_arc_dsc_t arc_dsc2;
    lv_draw_arc_dsc_init(&arc_dsc2);
    arc_dsc2.color = gray_color;
    arc_dsc2.width = 1;

    lv_draw_rect_dsc_t dot_dsc;
    lv_draw_rect_dsc_init(&dot_dsc);
    dot_dsc.bg_color = icon_color;
    dot_dsc.radius = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(signal_canvas, width / 2 - 2, height - 7, 4, 4, &dot_dsc); // 小圆点中心

    if (strength > 0)
    {
        lv_canvas_draw_arc(signal_canvas, width / 2, height - 5, 6, 240, 300, &arc_dsc); // 内圈
    }
    else
    {
        lv_canvas_draw_arc(signal_canvas, width / 2, height - 5, 6, 240, 300, &arc_dsc2); // 最外圈
    }

    if (strength > 1)
    {
        lv_canvas_draw_arc(signal_canvas, width / 2, height - 5, 10, 225, 315, &arc_dsc); // 中圈
    }
    else
    {
        lv_canvas_draw_arc(signal_canvas, width / 2, height - 5, 10, 225, 315, &arc_dsc2); // 最外圈
    }
    // 一共画三段弧表示信号
    if (strength > 2)
    {
        lv_canvas_draw_arc(signal_canvas, width / 2, height - 5, 14, 220, 320, &arc_dsc); // 最外圈
    }
    else
    {
        lv_canvas_draw_arc(signal_canvas, width / 2, height - 5, 14, 220, 320, &arc_dsc2); // 最外圈
    }

    if (strength == 0)
    {
        // 没信号
        int size = 8;
        lv_draw_line_dsc_t line_dsc;
        lv_draw_line_dsc_init(&line_dsc);
        line_dsc.color = lv_palette_main(LV_PALETTE_RED);
        line_dsc.width = 1;

        // 定义 X 的两条线
        lv_point_t line1[] = {{0, 0}, {size, size}};
        lv_point_t line2[] = {{0, size}, {size, 0}};
        // 在 canvas 上绘制
        lv_canvas_draw_line(signal_canvas, line1, 2, &line_dsc);
        lv_canvas_draw_line(signal_canvas, line2, 2, &line_dsc);
    }
    xSemaphoreGive(label_mutex);
}
/**
 * 信号渲染
 * @param strength 0-3
 */
void EEUI::render_signal(int strength)
{
    if (is_set_pure_mode)
        return;

    xSemaphoreTake(label_mutex, portMAX_DELAY);
    // 等待LVGL初始化完成
    await_lvgl_initialized();
    int width = signal_width;
    int height = signal_height;
    if (!signal_canvas)
    {
        signal_cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(signal_width, signal_height));
        signal_canvas = lv_canvas_create(lv_scr_act());
        lv_canvas_set_buffer(signal_canvas, signal_cbuf, width, height, LV_IMG_CF_TRUE_COLOR);
    }

    int right = screen_width - bat_width - width - get_left_right_padding() / 2 - 7;
    lv_obj_set_pos(signal_canvas, right, lv_obj_get_style_pad_top(containe_bt, 0) - 3);

    auto *param = new RenderBatParam{this, strength};
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<RenderBatParam *>(p);
                      param->self->render_signal_todo(param->percent);
                      delete param; // 释放内存
                  },
                  param);
}

void EEUI::render_signal_conneact_ani(int x)
{
    while (x >= 0)
    {
        render_signal(1);
        vTaskDelay(200 / portTICK_PERIOD_MS); // 等待屏幕稳定
        render_signal(2);
        vTaskDelay(200 / portTICK_PERIOD_MS); // 等待屏幕稳定
        render_signal(3);
        vTaskDelay(200 / portTICK_PERIOD_MS); // 等待屏幕稳定
        x -= 1;
    }
}

static void set_temp(void *bar, int32_t temp)
{
    lv_bar_set_value((lv_obj_t *)bar, temp, LV_ANIM_ON);
}

struct HideVolProps
{
    EEUI *self;
    lv_obj_t *data;
    lv_obj_t *data2;
};

void EEUI::render_volume_todo(float _volume)
{
    int width = volume_width;
    int height = volume_height;

    if (!volume_canvas)
    {
        volume_cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height));
        volume_canvas = lv_canvas_create(lv_scr_act());
        lv_canvas_set_buffer(volume_canvas, volume_cbuf, width, height, LV_IMG_CF_TRUE_COLOR);

        int offset_right = screen_width - bat_width - signal_width - width - get_left_right_padding() / 2 - 10; // 12 是边距

        // if (is_circle_screen)
        // {
        //     offset_right = screen_width / 2;
        //     lv_obj_set_pos(volume_canvas, offset_right - 45, lv_obj_get_style_pad_top(containe_bt, 0) - 3);
        // }
        // else
        // {

        lv_obj_set_pos(volume_canvas, offset_right, lv_obj_get_style_pad_top(containe_bt, 0) - 3);
        // }
    }

    // 清空 canvas
    lv_canvas_fill_bg(volume_canvas, lv_color_make(0xFF, 0xFF, 0xFF), LV_OPA_COVER);

    // 创建边框样式
    // lv_draw_rect_dsc_t rect_dsc_border;
    // lv_draw_rect_dsc_init(&rect_dsc_border);
    // rect_dsc_border.bg_opa = LV_OPA_TRANSP;          // 不填充背景，只画边
    // rect_dsc_border.border_color = lv_color_black(); // 边框颜色
    // rect_dsc_border.border_width = 1;                // 边框宽度
    // lv_canvas_draw_rect(volume_canvas, 0, 0, width, height, &rect_dsc_border);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.color = icon_color;
    label_dsc.font = LV_FONT_DEFAULT;

    lv_canvas_draw_text(
        volume_canvas,
        3, 4, // x, y 坐标
        50,   // 最大宽度
        &label_dsc,
        LV_SYMBOL_MUTE);

    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = icon_color;
    arc_dsc.width = 2;

    if (_volume >= 0.9)
    {
        lv_canvas_draw_arc(volume_canvas, width / 2 - 1, height / 2, 9, -65, 65, &arc_dsc); // 最外圈
    }
    if (_volume >= 0.5)
    {
        lv_canvas_draw_arc(volume_canvas, width / 2 - 3, height / 2, 7, -50, 50, &arc_dsc); // 中圈
    }
    if (_volume >= 0.2)
    {
        lv_canvas_draw_arc(volume_canvas, width / 2 - 5, height / 2, 5, -30, 30, &arc_dsc); // 内圈
    }
    // 音量动画
    if (!volume_bar)
    {
        static lv_style_t volume_bar_style_indic;
        lv_style_init(&volume_bar_style_indic);
        lv_style_set_bg_opa(&volume_bar_style_indic, LV_OPA_COVER);
        lv_style_set_bg_color(&volume_bar_style_indic, grad_bg_color1_volume);
        lv_style_set_bg_grad_color(&volume_bar_style_indic, grad_bg_color2_volume);
        lv_style_set_bg_grad_dir(&volume_bar_style_indic, LV_GRAD_DIR_VER);

        volume_bar = lv_bar_create(containe_bt);
        lv_obj_add_style(volume_bar, &volume_bar_style_indic, LV_PART_INDICATOR);
        lv_obj_set_size(volume_bar, 20, 140);
        lv_bar_set_range(volume_bar, 0, 100);
        lv_obj_move_foreground(volume_bar);
    }
    // 会被隐藏，每次都需要重新计算位置
    lv_obj_align(volume_bar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_move_foreground(volume_canvas);

    lv_anim_t volume_bar_ani;
    lv_anim_init(&volume_bar_ani);
    lv_anim_set_exec_cb(&volume_bar_ani, set_temp);
    lv_anim_set_time(&volume_bar_ani, 500);
    lv_anim_set_var(&volume_bar_ani, volume_bar);
    lv_anim_set_values(&volume_bar_ani, volume, (int)(_volume * 100));
    lv_anim_start(&volume_bar_ani);

    // label
    if (!volume_bar_label)
    {
        volume_bar_label = lv_label_create(containe_bt);
    }
    lv_label_set_text_fmt(volume_bar_label, "%d%%", (int)(_volume * 100));
    lv_obj_align_to(volume_bar_label, volume_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5); // 相对于 bar 向下偏移 5px
    lv_obj_move_foreground(volume_bar_label);

    // 隐藏
    if (volume_hide_timer)
    {
        lv_timer_del(volume_hide_timer);
        volume_hide_timer = NULL;
    }
    auto *param = new HideVolProps{this, volume_bar, volume_bar_label};
    volume_hide_timer = lv_timer_create(
        [](lv_timer_t *timer)
        {
            HideVolProps *props = static_cast<HideVolProps *>(timer->user_data);
            EEUI *self = props->self;
            lv_obj_t *data = props->data;
            lv_obj_t *data2 = props->data2;
            self->hide_status_gif(data);
            lv_label_set_text(data2, "");
            delete props;
            lv_timer_del(timer);
            self->volume_hide_timer = NULL;
        },
        1500,
        param);
    volume = (int)(_volume * 100);
    xSemaphoreGive(label_mutex);
}

struct RenderVolParam
{
    EEUI *self;
    float volume;
};
/**
 * 音量渲染
 * @param volume 0-1
 */
void EEUI::render_volume(float _volume)
{
    if (is_set_pure_mode)
        return;

    xSemaphoreTake(label_mutex, portMAX_DELAY);
    // 等待LVGL初始化完成
    await_lvgl_initialized();
    auto *param = new RenderVolParam{this, _volume};
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<RenderVolParam *>(p);
                      param->self->render_volume_todo(param->volume);
                      delete param; },
                  param);
}

/**
 * OTA 升级动画
 * 需要做节流
 */
static long prev_update_percent_time = 0;
struct OtaPercentProps
{
    int percent;
    lv_obj_t *containe_bt;
    lv_obj_t **ota_label;
    lv_obj_t **ota_bar;
};

void EEUI::render_ota_percent(int percent)
{
    // 等待LVGL初始化完成
    await_lvgl_initialized();

    long now = millis();
    if ((now - prev_update_percent_time) < 1500)
    {
        return;
    }
    prev_update_percent_time = now; 
    xSemaphoreTake(gif_render_ing, portMAX_DELAY);

    // 如果已存在，先删除旧的
    if (emo_img != NULL)
    {

        if (emo_img && lv_obj_is_valid(emo_img))
        {
            xSemaphoreTake(gif_show_ani_ing, portMAX_DELAY); 
            safe_delete_obj(emo_img);
            emo_img = NULL;
            xSemaphoreGive(gif_show_ani_ing);
        }
    }

    if (!ota_bar)
    {
        set_status_text("升级中", true);
    }
    auto *otaPercentProps = new OtaPercentProps{
        percent,
        containe_bt,
        &ota_label,
        &ota_bar};

    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<OtaPercentProps *>(p);

                      if (!*param->ota_bar)
                      {
                          *param->ota_label = lv_label_create(param->containe_bt);
                          lv_obj_set_style_text_font(*param->ota_label, &lv_font_montserrat_24, 0);
                          lv_obj_center(*param->ota_label);

                          *param->ota_bar = lv_arc_create(param->containe_bt);
                          lv_arc_set_rotation(*param->ota_bar, 270);
                          lv_arc_set_bg_angles(*param->ota_bar, 0, 360);
                          lv_obj_remove_style(*param->ota_bar, NULL, LV_PART_KNOB);
                          lv_obj_clear_flag(*param->ota_bar, LV_OBJ_FLAG_CLICKABLE);
                          lv_obj_center(*param->ota_bar);
                      }
 
                      lv_arc_set_value(*param->ota_bar, param->percent);
                      lv_label_set_text_fmt(*param->ota_label, "%d%%", param->percent);
 
                      delete param; // 防止内存泄漏
                      xSemaphoreGive(gif_render_ing); },
                  otaPercentProps); 
}

void EEUI::render_loading_todo()
{
    if (emo_img)
    {
        if (emo_img && lv_obj_is_valid(emo_img))
        {
            // lv_obj_del(emo_img);
            safe_delete_obj(emo_img);
        }
    }
    emo_img = lv_spinner_create(containe_bt, 1000, 60);
    lv_obj_set_size(emo_img, 120, 120);
    lv_obj_center(emo_img);
    xSemaphoreGive(gif_render_ing);
}

/**
 * Loading 动画渲染
 */
void EEUI::render_loading()
{
    xSemaphoreTake(gif_render_ing, portMAX_DELAY);
    auto *param = new RenderVolParam{this, NULL};
    lv_async_call([](void *p)
                  {
                      auto *param = static_cast<RenderVolParam *>(p);
                      param->self->render_loading_todo();
                      delete param; },
                  param);
}

void EEUI::hide_loading()
{
    if (emo_img)
    {
        if (emo_img && lv_obj_is_valid(emo_img))
        {
            // lv_obj_del(emo_img);
            safe_delete_obj(emo_img);
        }
        emo_img = NULL;
    }
}

/**
 * 闹钟渲染
 */
void EEUI::render_clock()
{
    if (is_set_pure_mode)
        return;

    xSemaphoreTake(gif_render_ing, portMAX_DELAY);
    // 等待LVGL初始化完成
    await_lvgl_initialized();

    const int width = 22;  // 闹钟图标宽度
    const int height = 22; // 闹钟图标高度

    // 创建 canvas
    if (!clock_canvas)
    {
        clock_cbuf = (lv_color_t *)malloc(LV_CANVAS_BUF_SIZE_TRUE_COLOR(width, height));
        clock_canvas = lv_canvas_create(containe_bt);
        lv_canvas_set_buffer(clock_canvas, clock_cbuf, width, height, LV_IMG_CF_TRUE_COLOR);
    }

    // 计算位置 - 放在右上角电池图标左边
    int right = screen_width - bat_width - signal_width - volume_width - width - get_left_right_padding() / 2 - 26;
    lv_obj_set_pos(clock_canvas, right, -5);

    // 清空画布
    lv_canvas_fill_bg(clock_canvas, lv_color_make(0xFF, 0xFF, 0xFF), LV_OPA_COVER);

    // 画圆形表盘
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = icon_color;
    arc_dsc.width = 2;
    lv_canvas_draw_arc(clock_canvas, width / 2, height / 2, width / 2 - 2, 0, 360, &arc_dsc);

    // 画上方的闹钟"耳朵"
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = icon_color;
    line_dsc.width = 1;

    // 画时钟指针
    lv_point_t hour_hand[] = {
        {width / 2, height / 2},
        {width / 2 - 3, height / 2 - 2}};
    lv_canvas_draw_line(clock_canvas, hour_hand, 2, &line_dsc);

    lv_point_t minute_hand[] = {
        {width / 2, height / 2},
        {width / 2 + 5, height / 2}};
    lv_canvas_draw_line(clock_canvas, minute_hand, 2, &line_dsc);
    xSemaphoreGive(gif_render_ing);
}

/**
 * 隐藏闹钟渲染
 */
void EEUI::hide_clock()
{
    if (is_set_pure_mode)
        return;

    xSemaphoreTake(gif_render_ing, portMAX_DELAY);

    // 等待LVGL初始化完成
    await_lvgl_initialized();

    if (clock_canvas && lv_obj_is_valid(clock_canvas))
    {
        lv_canvas_fill_bg(clock_canvas, lv_color_make(0xFF, 0xFF, 0xFF), LV_OPA_COVER);
    }
    xSemaphoreGive(gif_render_ing);
}
