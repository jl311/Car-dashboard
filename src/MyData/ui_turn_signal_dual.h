/**
 * @file ui_turn_signal_dual.h
 * @brief 转向灯闪烁功能（双闪扩展版）
 *
 * 相比基础版(ui_turn_signal)：
 *  - 去掉单枚举状态机，改为 left_on / right_on 两个独立标志
 *  - 每侧各一个独立定时器，互不影响 -> 左右可同时闪烁（双闪/危险报警）
 *  - 新增 ui_turn_signal_dual_toggle_both() 一键切换双闪
 *
 * 基础版用一个枚举 state + 一个定时器，导致左右互斥、不能同时亮；
 * 本版把"一个状态变量"升级成"两个独立状态 + 两个独立定时器"，
 * 每侧只翻自己那张图，互不干扰。
 */

#ifndef UI_TURN_SIGNAL_DUAL_H
#define UI_TURN_SIGNAL_DUAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"

/**
 * @brief 初始化转向灯闪烁功能（双闪版）
 *        左按钮 toggle 左灯，右按钮 toggle 右灯，两侧可同时闪。
 *
 * @param left_btn          左转向按钮对象
 * @param right_btn         右转向按钮对象
 * @param left_img          左转向灯图像对象
 * @param right_img         右转向灯图像对象
 * @param blink_interval_ms 闪烁间隔(毫秒) (建议: 300 ~ 600ms)
 */
void ui_turn_signal_dual_init(lv_obj_t * left_btn, lv_obj_t * right_btn,
                              lv_obj_t * left_img, lv_obj_t * right_img,
                              uint32_t blink_interval_ms);

/**
 * @brief 一键切换"左右同时闪"（危险报警双闪）。
 *        可在第三个按钮 / 菜单 / 长按事件里调用。
 */
void ui_turn_signal_dual_toggle_both(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_TURN_SIGNAL_DUAL_H */
