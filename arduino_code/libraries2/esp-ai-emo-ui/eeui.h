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
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "font/font_chinese_16.c"
// #include "imgs/rechargeing.h"
#include "esp_timer.h"

struct EEUIEmotionImagePair
{
    String emotion_name;
    const lv_img_dsc_t *image;
};

class EEUI
{
public:
    EEUI();
    void begin(TFT_eSPI *tft, const EEUIEmotionImagePair *emotions, size_t emotion_count, int screen_width, int screen_height, int screen_pad_left, int screen_pad_right);

    /**
     * 设置为简洁模式,只显示图片
     */
    void set_pure_mode();

    /**
     * 设置主题色，必须在所有操作前e设置才u有效，或者调用重新渲染方法
     * eg: eeui.set_theme_color();
     */
    void set_theme_color(const lv_color_t color);

    /**
     * 设置背景颜色,必须在 begin 之前调用才会生效
    */
    void set_eeui_bg_color(const lv_color_t color);

    /**
     * 直接渲染 GIF 图片
     * eg: eeui.render_gif(emotions[0].image);
     * @param need_ani 需要执行出场动画
     */
    void render_gif(const lv_img_dsc_t *image, bool need_ani = false, lv_coord_t x_ofs = 0, lv_coord_t y_ofs = 0);

    /**
     * 渲染传入的表情Map中的某个Gif表情
     * eg: eeui.render_gif_by_name("快乐");
     */
    void render_gif_by_name(const char *image_name, lv_coord_t x_ofs = 0, lv_coord_t y_ofs = 0); 

    /**
     * 渲染设备状态
     * @param text 要显示的文字内容
     * @param need_ani 是否需要出场动画效果
     * @param align 对齐方式  "top_left" | "bottom_center"
     */
    void set_status_text(const char *text, bool need_ani, const char *align);
    
    /**
     * 在指定位置显示状态文本
     * @param text 要显示的文本内容
     * @param need_ani 是否需要动画效果
     * @param x 文本的X坐标位置
     * @param y 文本的Y坐标位置
     */
void set_status_text_positioned(const char *text, bool need_ani = true, const char *align = "top_left", lv_coord_t x = 0, lv_coord_t y = 0);
    /**
     * 渲染底部滚动文字
     */
    void set_bottom_scrolling_text(const char *text);

    /**
     * 渲染电量
     * 0-100 范围
     */
    void render_battery(int percent);
    /**
     * 隐藏电量显示
     */
    void hide_battery();

    /**
     * 设置是否在充电中
     */
    void recharge(bool is_recharge);

    /**
     * 等待LVGL初始化完成
     */
    void await_lvgl_initialized();

    /**
     * 让所有文字保持在最上层显示
     */
    void move_text_top();

    /**
     * 信号渲染
     * 信号强度： 0-3
     */
    void render_signal(int strength);

    /**
     * 执行x遍WIFI连接动画
     */
    void render_signal_conneact_ani(int x);

    /**
     * 音量渲染
     * @param volume 0-1
     */
    void render_volume(float volume);

    /**
     * OTA 升级进度
     * @param percent 0-100
     */
    void render_ota_percent(int percent);

    /**
     * Loading
     */
    void render_loading();
    void hide_loading();

    /**
     * 闹钟渲染
     */
    void render_clock();
    
    /**
     * 隐藏闹钟渲染
     */
    void hide_clock();

    /**
     * 渲染播放/暂停图标
     * @param is_playing true=播放中(显示暂停图标), false=暂停(显示播放图标)
     */
    void render_play_icon(bool is_playing);

    /**
     * 渲染蓝牙连接图标
     * @param is_connected true=已连接, false=未连接
     */
    void render_bluetooth_icon(bool is_connected);

    /**
     * 显示歌曲信息（屏幕中央）
     * @param title 歌曲标题
     * @param artist 艺术家名称（可选）
     */
    void render_song_info(const char *title, const char *artist = nullptr);

    /**
     * 隐藏歌曲信息
     */
    void hide_song_info();

    /**
     * 渲染圆形旋转图片（专辑封面）
     * @param image 图片数据指针
     * @param is_playing 是否正在播放（控制旋转）
     */
    void render_rotating_image(const lv_img_dsc_t *image, bool is_playing);

    /**
     * 隐藏圆形旋转图片
     */
    void hide_rotating_image();

    /**
     * 更新旋转状态（播放/暂停）
     * @param is_playing 是否正在播放
     */
    void update_rotation_state(bool is_playing);

private:
    TFT_eSPI *tft;
    int screen_width = 240;
    int screen_height = 240;
    int screen_pad_left = 0;
    int screen_pad_right = 0;
    bool lvgl_initialized = false;

    // 主题色: 主体颜色，包括文字等
    lv_color_t theme_color = lv_color_make(25, 50, 83);
    lv_color_t gray_color = lv_color_make(215, 215, 215);
    lv_color_t gray_color2 = lv_color_make(150, 150, 150);
    lv_color_t icon_color = lv_color_make(100, 100, 100);

    // 渐变颜色
    lv_color_t grad_bg_color1 = lv_color_make(138, 252, 155); // 上
    lv_color_t grad_bg_color2 = lv_color_make(24, 250, 217);  // 下

    // 音量渐变颜色
    lv_color_t grad_bg_color1_volume = lv_color_make(118, 75, 162);  // 上
    lv_color_t grad_bg_color2_volume = lv_color_make(102, 126, 234); // 下
    void render_volume_todo(float volume);

    lv_disp_draw_buf_t draw_buf;
    lv_color_t buf[LV_HOR_RES_MAX * 15];

    // 表情
    const EEUIEmotionImagePair *emotions = nullptr;
    size_t emotion_count = 0;

    // 电量
    int bat_percent = 0;
    lv_obj_t *battery_canvas = nullptr;
    lv_color_t *battery_cbuf = nullptr;

    lv_obj_t *emo_img = nullptr;
    lv_timer_t *hide_timer = nullptr;
    lv_obj_t *containe_bt = nullptr;

    // 定时器相关变量
    hw_timer_t *timer = nullptr;
    static portMUX_TYPE timerMux;

    // 闹钟
    lv_obj_t *clock_canvas = NULL;
    lv_color_t *clock_cbuf = NULL;

    // 定义底部滚动标签
    lv_obj_t *botom_scroll_label = nullptr;
    lv_style_t botom_scroll_label_style;

    /**
     * 滑动出现底部文本
     */
    void show_bottom_text(lv_obj_t *label);
    void set_bottom_scrolling_text_todo(const char *text);

    // 定义状态文字滚动标签
    lv_obj_t *status_scroll_label = nullptr;
    lv_style_t status_scroll_label_style;

    /**
     * 滑动出现状态文字
     */
    void show_status_text(lv_obj_t *label);
    void set_status_text_todo(const char *text, bool need_ani, const char *align);
    
    /**
     * 设置状态文本并指定显示位置
     */
    void set_status_text_with_position(const char *text, bool need_ani, const char *align, lv_coord_t x, lv_coord_t y);

    /**
     * 向左侧滑动隐藏文本
     */
    void hide_text_left(lv_obj_t *label);

    /**
     * 容器设置
     */
    void init_container();

    static void IRAM_ATTR onTimer();
    static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    void render_gif_todo(const lv_img_dsc_t *image, bool need_ani,lv_coord_t x_ofs, lv_coord_t y_ofs); 

    // 获取屏幕左右两侧的内边距
    int get_left_right_padding();
    // 获取屏幕上下两侧的内边距
    int get_top_bottom_padding();

    // 电量相关
    int bat_width = 28;
    int bat_height = 14; 

    int get_bat_x()
    {
        // if (is_circle_screen)
        // {
        //     return screen_width / 2 - 30;
        // }
        return screen_width - get_left_right_padding() / 2 - bat_width;
    }
    int get_bat_y()
    {
        // if (is_circle_screen)
        // {
        //     return screen_height - 40;
        // }
        return get_top_bottom_padding() / 2;
    }

    void render_battery_todo(int percent);
    bool is_recharge = false;

    // 信号相关
    int signal_width = 28;
    int signal_height = 22;
    lv_color_t *signal_cbuf = nullptr;
    lv_obj_t *signal_canvas = nullptr;
    void render_signal_todo(int strength);
    int signal = 0;

    // 音量相关
    int volume = 0;
    int volume_width = 26;
    int volume_height = 22;
    lv_color_t *volume_cbuf = nullptr;
    lv_obj_t *volume_canvas = nullptr;
    lv_obj_t *volume_bar = nullptr;
    lv_obj_t *volume_bar_label = nullptr;
    lv_timer_t *volume_hide_timer = nullptr;

    // 状态动画相关
    SemaphoreHandle_t status_gif_ani_mutex = xSemaphoreCreateMutex();
    lv_timer_t *hide_status_gif_timer = nullptr;
    void show_status_gif(lv_obj_t *img);
    void hide_status_gif(lv_obj_t *img);

    // OTA 动画相关
    lv_obj_t *ota_bar;
    lv_obj_t *ota_label;

    void render_loading_todo();

    // 播放图标相关
    int play_icon_width = 20;
    int play_icon_height = 20;
    lv_color_t *play_icon_cbuf = nullptr;
    lv_obj_t *play_icon_canvas = nullptr;
    void render_play_icon_todo(bool is_playing);

    // 蓝牙图标相关
    int bt_icon_width = 20;
    int bt_icon_height = 20;
    lv_color_t *bt_icon_cbuf = nullptr;
    lv_obj_t *bt_icon_canvas = nullptr;
    void render_bluetooth_icon_todo(bool is_connected);

    // 歌曲信息相关
    lv_obj_t *song_title_label = nullptr;
    lv_obj_t *song_artist_label = nullptr;
    lv_style_t song_title_style;
    lv_style_t song_artist_style;
    void render_song_info_todo(const char *title, const char *artist);

    // 圆形旋转图片相关
    lv_obj_t *rotating_image = nullptr;      // 旋转图片对象
    lv_anim_t rotation_anim;                 // 旋转动画对象
    bool is_rotation_active = false;         // 旋转是否激活
    int rotating_image_size = 160;           // 图片尺寸
    void render_rotating_image_todo(const lv_img_dsc_t *image, bool is_playing);
    void hide_rotating_image_todo();
    void update_rotation_state_todo(bool is_playing);
    static void rotation_anim_cb(void *var, int32_t value); // 旋转动画回调
};
