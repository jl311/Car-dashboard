/**
 * @file ui_turn_signal_dual.c
 * @brief 转向灯闪烁（双闪扩展版）
 *
 * 基础版(ui_turn_signal.c)用一个枚举 state + 一个定时器，
 * 导致左右互斥、不能同时亮。
 *
 * 本版的做法（对应课堂讲的"扩展 1：双闪"）：
 *   1. 把单枚举 state 换成 left_on / right_on 两个独立 bool 标志；
 *   2. 每侧各用一个独立定时器，各自只翻转自己那张图的 HIDDEN 标志；
 *   3. 两侧互不干扰 -> 可同时闪烁（危险报警双闪）。
 *
 * 关键认知：闪烁本质 = 周期性翻转 LV_OBJ_FLAG_HIDDEN。
 */

#include "ui_turn_signal_dual.h"
#include <stdbool.h>

/* 双闪版上下文：两个独立标志 + 两个独立定时器 */
typedef struct {
    lv_obj_t  * left_img;            // 左转向灯图像
    lv_obj_t  * right_img;           // 右转向灯图像
    uint32_t    blink_interval_ms;   // 闪烁间隔(毫秒)
    bool        left_on;             // 左灯是否在闪（独立状态）
    bool        right_on;            // 右灯是否在闪（独立状态）
    lv_timer_t * left_timer;         // 左灯定时器（独立）
    lv_timer_t * right_timer;        // 右灯定时器（独立）
} turn_signal_dual_ctx_t;

/* 全局上下文（仅一组转向灯） */
static turn_signal_dual_ctx_t g_ctx_dual;

/* 左灯闪烁定时器回调：只翻左图，不碰右图 */
static void left_blink_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    if (!g_ctx_dual.left_img) return;

    if (lv_obj_has_flag(g_ctx_dual.left_img, LV_OBJ_FLAG_HIDDEN))
        lv_obj_remove_flag(g_ctx_dual.left_img, LV_OBJ_FLAG_HIDDEN);  // 藏 -> 显
    else
        lv_obj_add_flag(g_ctx_dual.left_img, LV_OBJ_FLAG_HIDDEN);     // 显 -> 藏
}

/* 右灯闪烁定时器回调：只翻右图，不碰左图 */
static void right_blink_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    if (!g_ctx_dual.right_img) return;

    if (lv_obj_has_flag(g_ctx_dual.right_img, LV_OBJ_FLAG_HIDDEN))
        lv_obj_remove_flag(g_ctx_dual.right_img, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(g_ctx_dual.right_img, LV_OBJ_FLAG_HIDDEN);
}

/* 启动某一侧闪烁（不影响另一侧） */
static void start_side(bool is_left)
{
    lv_obj_t   * img   = is_left ? g_ctx_dual.left_img  : g_ctx_dual.right_img;
    lv_timer_t ** pt   = is_left ? &g_ctx_dual.left_timer  : &g_ctx_dual.right_timer;
    bool       * pon  = is_left ? &g_ctx_dual.left_on     : &g_ctx_dual.right_on;

    if (*pon) return;                       /* 已经在闪，不重复建定时器 */
    if (*pt) { lv_timer_del(*pt); *pt = NULL; }

    lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);   /* 初始先显示 */
    *pt = lv_timer_create(is_left ? left_blink_timer_cb : right_blink_timer_cb,
                          g_ctx_dual.blink_interval_ms, NULL);
    if (*pt) lv_timer_set_repeat_count(*pt, -1);   /* 无限循环 */
    *pon = true;
}

/* 停止某一侧闪烁（停后该侧常亮；想"灭"把下面 remove 改成 add） */
static void stop_side(bool is_left)
{
    lv_obj_t   * img  = is_left ? g_ctx_dual.left_img  : g_ctx_dual.right_img;
    lv_timer_t ** pt  = is_left ? &g_ctx_dual.left_timer  : &g_ctx_dual.right_timer;
    bool       * pon  = is_left ? &g_ctx_dual.left_on     : &g_ctx_dual.right_on;

    if (*pt) { lv_timer_del(*pt); *pt = NULL; }
    lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);       /* 常亮（不闪） */
    *pon = false;
}

/* 左按钮：toggle 左灯 */
static void left_btn_click_cb(lv_event_t * e)
{
    (void)e;
    if (g_ctx_dual.left_on) stop_side(true);
    else                    start_side(true);
}

/* 右按钮：toggle 右灯 */
static void right_btn_click_cb(lv_event_t * e)
{
    (void)e;
    if (g_ctx_dual.right_on) stop_side(false);
    else                     start_side(false);
}

void ui_turn_signal_dual_init(lv_obj_t * left_btn, lv_obj_t * right_btn,
                              lv_obj_t * left_img, lv_obj_t * right_img,
                              uint32_t blink_interval_ms)
{
    if (!left_btn || !right_btn || !left_img || !right_img) return;
    if (blink_interval_ms < 100) blink_interval_ms = 100;

    /* 清理可能残留的旧定时器 */
    if (g_ctx_dual.left_timer)  { lv_timer_del(g_ctx_dual.left_timer);  g_ctx_dual.left_timer = NULL; }
    if (g_ctx_dual.right_timer) { lv_timer_del(g_ctx_dual.right_timer); g_ctx_dual.right_timer = NULL; }

    g_ctx_dual.left_img  = left_img;
    g_ctx_dual.right_img = right_img;
    g_ctx_dual.blink_interval_ms = blink_interval_ms;
    g_ctx_dual.left_on  = false;
    g_ctx_dual.right_on = false;

    lv_obj_remove_flag(left_img,  LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(right_img, LV_OBJ_FLAG_HIDDEN);

    /* 两个按钮各自只 toggle 自己那侧，互不影响 */
    lv_obj_add_event_cb(left_btn,  left_btn_click_cb,  LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(right_btn, right_btn_click_cb, LV_EVENT_CLICKED, NULL);
}

/* 一键双闪：两侧同时 toggle */
void ui_turn_signal_dual_toggle_both(void)
{
    bool both_on = g_ctx_dual.left_on && g_ctx_dual.right_on;
    if (both_on) { stop_side(true);  stop_side(false); }
    else         { start_side(true); start_side(false); }
}
