#ifndef UI_ARC_BIND_H
#define UI_ARC_BIND_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 绑定圆弧、标签、图片，指定数值映射范围
 * @param arc        圆弧对象
 * @param label      标签对象（显示数值）
 * @param img        图片对象（旋转）
 * @param start_deg  最小值对应的角度（1/10度）
 * @param end_deg    最大值对应的角度（1/10度）
 * @param min_val    圆弧最小值（映射起点）
 * @param max_val    圆弧最大值（映射终点）
 */
void ui_arc_bind(lv_obj_t * arc, lv_obj_t * label, lv_obj_t * img,
                 int32_t start_deg, int32_t end_deg,
                 int32_t min_val, int32_t max_val);

/**
 * @brief 根据圆弧当前值立即更新绑定的标签和图片
 * @param arc  圆弧对象（必须已绑定）
 */
void ui_arc_bind_update(lv_obj_t * arc);

#ifdef __cplusplus
}
#endif

#endif