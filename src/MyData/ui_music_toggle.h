#ifndef UI_MUSIC_TOGGLE_H
#define UI_MUSIC_TOGGLE_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化音乐播放器
 * @param play_icon   播放/暂停按钮
 * @param prev_icon   上一首按钮
 * @param next_icon   下一首按钮
 * @param playlist    歌单数组（MP3 路径）
 * @param cover_list  封面图数组（PNG 路径），与 playlist 一一对应
 * @param count       歌曲数量
 */
void ui_music_init(lv_obj_t *play_icon,
                   lv_obj_t *prev_icon,
                   lv_obj_t *next_icon,
                   const char **playlist,
                   const char **cover_list,
                   int count);

/**
 * @brief 设置封面图片控件（在 Screen2 上居中的 lv_image 对象）
 */
void ui_music_set_cover(lv_obj_t *cover_img);

/**
 * @brief 设置进度条滑块
 */
void ui_music_set_slider(lv_obj_t *slider);

/**
 * @brief 释放资源
 */
void ui_music_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif