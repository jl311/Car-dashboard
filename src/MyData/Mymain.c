#include <stdio.h>
#include <time.h>
#include "../../lvgl/lvgl.h"
#include "src/MyData/ui_arc_auto_decrease.h"
#include "src/MyData/ui_turn_signal.h"
#include "src/MyData/ui/ui.h"
#include "src/MyData/ui_arc_bind.h"
#include "src/MyData/ui_turn_signal_dual.h"
#include "src/MyData/ui_music_toggle.h"

void ui_init(void);

/* ===================== 全局数据 ===================== */

/* 屏幕2时钟 */
static lv_obj_t * clock_label_screen2 = NULL;

/* 歌单 */
static const char *music_playlist[] = {
    "/IOT/ui/try/music/music.mp3",
    "/IOT/ui/try/music/music2.mp3",
    // 可以继续添加
};

/* 封面（与歌单一一对应） */
static const char *music_covers[] = {
    "/IOT/ui/try/image/image1.png",
    "/IOT/ui/try/image/image2.png",
    // 可以继续添加
};

/* 封面控件（在 Screen2 上居中显示） */
static lv_obj_t *music_cover = NULL;

/* ===================== 屏幕2时钟 ===================== */

static void update_clock_screen2_cb(lv_timer_t * timer) {
    (void)timer;
    if (!clock_label_screen2) return;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M", tm_info);
    lv_label_set_text(clock_label_screen2, buf);
}

static void create_clock_on_screen2(void) {
    if (!ui_Screen2) return;
    clock_label_screen2 = lv_label_create(ui_Screen2);
    lv_label_set_text(clock_label_screen2, "00:00");
    lv_obj_set_style_text_font(clock_label_screen2, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(clock_label_screen2, lv_color_hex(0x00FF00), 0);
    lv_obj_align(clock_label_screen2, LV_ALIGN_TOP_MID, 0, 20);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M", tm_info);
    lv_label_set_text(clock_label_screen2, buf);

    lv_timer_t * timer = lv_timer_create(update_clock_screen2_cb, 60000, NULL);
    if (timer) lv_timer_set_repeat_count(timer, -1);
}

/* ===================== 启动动画淡出 ===================== */

static void opa_anim_cb(void * var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)var, v, 0);
}

static void splash_fade_ready_cb(lv_anim_t * anim) {
    lv_obj_t * container = (lv_obj_t *)anim->var;
    if (container) lv_obj_delete(container);
}

static void splash_fade_start_cb(lv_timer_t * timer) {
    lv_obj_t * container = (lv_obj_t *)lv_timer_get_user_data(timer);
    if (!container) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, container);
    lv_anim_set_exec_cb(&a, opa_anim_cb);
    lv_anim_set_values(&a, 255, 0);
    lv_anim_set_time(&a, 300);
    lv_anim_set_ready_cb(&a, splash_fade_ready_cb);
    lv_anim_start(&a);
}

/* ===================== 主入口 ===================== */

void mymain(void)
{
    /* ---------- 1. 创建所有屏幕 ---------- */
    ui_init();

    /* ---------- 2. 音乐播放器 ---------- */
    // 在 Screen2 上创建一个居中的封面图片控件（320x240）
    music_cover = lv_image_create(ui_Screen2);
    lv_obj_set_size(music_cover, 320, 240);
    lv_obj_center(music_cover);

    ui_music_init(ui_Image15,          // 播放/暂停
                  ui_Image16,          // 上一首
                  ui_Image17,          // 下一首
                  music_playlist,
                  music_covers,
                  sizeof(music_playlist) / sizeof(music_playlist[0]));

    ui_music_set_cover(music_cover);   // 绑定封面
    ui_music_set_slider(ui_Slider1);   // 绑定进度条

    /* ---------- 3. 圆弧绑定 ---------- */
    ui_arc_bind(ui_Arc3, ui_Label5, ui_Image11, 400, 3200, 0, 1000);
    ui_arc_bind(ui_Arc2, ui_Label3, ui_Image5, 400, 3200, 0, 240);

    /* ---------- 4. 油门自动回落 ---------- */
    ui_arc_auto_decrease_init(ui_Arc3, ui_Image2, -8, 100);
    ui_arc_auto_decrease_init(ui_Arc2, ui_Image2, -2, 100);

    /* ---------- 5. 转向灯（双闪版） ---------- */
    ui_turn_signal_dual_init(ui_Button1, ui_Button2, ui_Image3, ui_Image4, 300);

    /* ---------- 6. 屏幕2时钟 ---------- */
    create_clock_on_screen2();

    /* ---------- 7. 启动动画（白色背景 + 居中 GIF + 淡出） ---------- */
    lv_obj_t * splash_container = lv_obj_create(lv_layer_top());
    lv_obj_set_size(splash_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_center(splash_container);
    lv_obj_set_style_bg_color(splash_container, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(splash_container, 255, 0);
    lv_obj_clear_flag(splash_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * splash_gif = lv_gif_create(splash_container);
    lv_gif_set_src(splash_gif, "A:love.gif");
    lv_obj_set_size(splash_gif, 242, 242);
    lv_obj_center(splash_gif);

    lv_timer_t * splash_timer = lv_timer_create(splash_fade_start_cb, 3000, splash_container);
    if (splash_timer) {
        lv_timer_set_repeat_count(splash_timer, 1);
    }
}