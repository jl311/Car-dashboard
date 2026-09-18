#include "ui_arc_bind.h"
#include <stdio.h>
#include <string.h>

#define MAX_BINDS 10

typedef struct {
    lv_obj_t * arc;
    lv_obj_t * label;
    lv_obj_t * img;
    int32_t start_deg;
    int32_t end_deg;
    int32_t min_val;
    int32_t max_val;
} bind_item_t;

static bind_item_t binds[MAX_BINDS];
static int bind_count = 0;

/* 查找绑定项 */
static bind_item_t * find_bind(lv_obj_t * arc) {
    for (int i = 0; i < bind_count; i++) {
        if (binds[i].arc == arc) return &binds[i];
    }
    return NULL;
}

/* 值改变事件回调（自动触发） */
static void value_changed_cb(lv_event_t * e) {
    lv_obj_t * arc = lv_event_get_target(e);
    ui_arc_bind_update(arc);
}

/* 绑定函数 */
void ui_arc_bind(lv_obj_t * arc, lv_obj_t * label, lv_obj_t * img,
                 int32_t start_deg, int32_t end_deg,
                 int32_t min_val, int32_t max_val) {
    if (!arc || !label || !img) return;

    // 参数合法性检查
    if (min_val >= max_val) return;
    if (start_deg == end_deg) return; // 避免除零

    // 如果已存在，更新配置
    bind_item_t * item = find_bind(arc);
    if (item) {
        item->label = label;
        item->img = img;
        item->start_deg = start_deg;
        item->end_deg = end_deg;
        item->min_val = min_val;
        item->max_val = max_val;
        ui_arc_bind_update(arc);
        return;
    }

    if (bind_count >= MAX_BINDS) return;

    binds[bind_count].arc = arc;
    binds[bind_count].label = label;
    binds[bind_count].img = img;
    binds[bind_count].start_deg = start_deg;
    binds[bind_count].end_deg = end_deg;
    binds[bind_count].min_val = min_val;
    binds[bind_count].max_val = max_val;
    bind_count++;

    // 注册事件，使拖动或点击时自动更新
    lv_obj_add_event_cb(arc, value_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 立即更新一次
    ui_arc_bind_update(arc);
}

/* 更新函数 */
void ui_arc_bind_update(lv_obj_t * arc) {
    bind_item_t * item = find_bind(arc);
    if (!item) return;

    int32_t val = lv_arc_get_value(arc);
    // 限幅到自定义范围
    if (val < item->min_val) val = item->min_val;
    if (val > item->max_val) val = item->max_val;

    // 更新标签
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d", val);
    lv_label_set_text(item->label, buf);

    // 线性映射角度
    int32_t range = item->max_val - item->min_val;
    int32_t deg_range = item->end_deg - item->start_deg;
    int32_t angle = item->start_deg + (val - item->min_val) * deg_range / range;
    // 限幅
    if (angle > item->end_deg) angle = item->end_deg;
    if (angle < item->start_deg) angle = item->start_deg;
    lv_image_set_rotation(item->img, angle);
}