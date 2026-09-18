#include "ui_music_toggle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

/* ===== 播放器状态 ===== */
static const char **g_playlist = NULL;
static const char **g_cover_list = NULL;
static int          g_count = 0;
static int          g_index = 0;
static int          g_playing = 0;
static int          g_duration_sec = 0;

/* ===== 封面图 ===== */
static lv_obj_t    *g_cover_img = NULL;

/* ===== 进度条 ===== */
static lv_obj_t    *g_slider = NULL;
static lv_timer_t  *g_progress_timer = NULL;
static struct timeval g_play_start;
static int          g_user_seeking = 0;
static int          g_grace_ticks = 0;

static void music_play_current(void);

/* ============================================================
 * 精确时长计算：逐帧扫描整个 MP3 文件
 * - 跳过 ID3v2 标签
 * - 遍历所有 MP3 帧，累加每帧样本数
 * - 时长 = 总样本数 / 采样率
 * 对 CBR / VBR 都准确
 * ============================================================ */
static int get_mp3_duration(const char *file) {
    FILE *fp = fopen(file, "rb");
    if (!fp) return 240;

    /* 读取前 10 字节，判断 ID3v2 标签 */
    unsigned char id3[10] = {0};
    if (fread(id3, 1, 10, fp) != 10) { fclose(fp); return 240; }

    long offset = 0;
    if (id3[0] == 'I' && id3[1] == 'D' && id3[2] == '3') {
        offset = 10
               + ((long)(id3[6] & 0x7F) << 21)
               + ((long)(id3[7] & 0x7F) << 14)
               + ((long)(id3[8] & 0x7F) << 7)
               +  (long)(id3[9] & 0x7F);
    }

    fseek(fp, offset, SEEK_SET);

    /* 比特率表（kbps），按 MPEG 版本和 Layer 分 */
    static const int br_v1l1[]  = {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0};
    static const int br_v1l2[]  = {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0};
    static const int br_v1l3[]  = {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0};
    static const int br_v2l1[]  = {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0};
    static const int br_v2l23[] = {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,0};

    long total_samples = 0;
    int  sample_rate   = 0;
    int  frame_count   = 0;

    unsigned char hdr[4];
    while (fread(hdr, 1, 4, fp) == 4) {
        /* 帧同步：11 位全 1 */
        if (hdr[0] != 0xFF || (hdr[1] & 0xE0) != 0xE0) {
            fseek(fp, -3, SEEK_CUR);   /* 退 3 字节继续找 */
            continue;
        }

        int b1 = hdr[1], b2 = hdr[2];
        int version     = (b1 >> 3) & 0x03;
        int layer       = (b1 >> 1) & 0x03;
        int bitrate_idx = (b2 >> 4) & 0x0F;
        int sample_idx  = (b2 >> 2) & 0x03;
        int padding     = (b2 >> 1) & 0x01;

        if (version == 1 || layer == 0 || bitrate_idx == 0 ||
            bitrate_idx == 15 || sample_idx == 3) {
            continue;
        }

        /* 比特率 */
        int bitrate = 128;
        if (version == 3) {           /* MPEG1 */
            if (layer == 3)      bitrate = br_v1l1[bitrate_idx];
            else if (layer == 2) bitrate = br_v1l2[bitrate_idx];
            else                 bitrate = br_v1l3[bitrate_idx];
        } else {                      /* MPEG2 / 2.5 */
            if (layer == 1)      bitrate = br_v2l23[bitrate_idx];
            else                 bitrate = br_v2l1[bitrate_idx];
        }
        if (bitrate <= 0) continue;

        /* 采样率 */
        int sr;
        if (version == 3)      sr = 44100;
        else if (version == 2) sr = 22050;
        else                   sr = 11025;
        if (sample_idx == 1)      sr /= 2;
        else if (sample_idx == 2) sr /= 4;
        else if (sample_idx != 0) continue;

        if (sample_rate == 0) sample_rate = sr;

        /* 每帧样本数 */
        int samples_per_frame;
        if (layer == 3)      samples_per_frame = 384;
        else if (layer == 2) samples_per_frame = 1152;
        else                 samples_per_frame = (version == 3) ? 1152 : 576;

        total_samples += samples_per_frame;
        frame_count++;

        /* 帧长度（字节） */
        int frame_size;
        if (layer == 3)
            frame_size = (12 * bitrate * 1000 / sr + padding) * 4;
        else if (layer == 2)
            frame_size = 144 * bitrate * 1000 / sr + padding;
        else
            frame_size = (version == 3)
                       ? (144 * bitrate * 1000 / sr + padding)
                       : ( 72 * bitrate * 1000 / sr + padding);

        if (frame_size < 4) break;
        fseek(fp, frame_size - 4, SEEK_CUR);
    }

    fclose(fp);

    if (total_samples > 0 && sample_rate > 0 && frame_count > 0) {
        int dur = (int)(total_samples / sample_rate);
        if (dur < 5)    dur = 5;
        if (dur > 7200) dur = 7200;
        printf("[音乐] 扫描 %d 帧, 采样率 %d Hz, 时长 %d 秒\n",
               frame_count, sample_rate, dur);
        return dur;
    }

    /* 兜底 */
    struct stat st;
    if (stat(file, &st) != 0) return 240;
    int dur = (int)(st.st_size / 16000);
    if (dur < 10) dur = 240;
    printf("[音乐] 兜底估算: %d 秒\n", dur);
    return dur;
}

/* ===== 更新封面 ===== */
static void update_cover(void) {
    if (!g_cover_img) return;
    if (g_cover_list && g_index < g_count && g_cover_list[g_index]) {
        lv_image_set_src(g_cover_img, g_cover_list[g_index]);
        printf("[音乐] 封面 -> %s\n", g_cover_list[g_index]);
    } else {
        lv_image_set_src(g_cover_img, NULL);
    }
}

/* ===== 停止 madplay ===== */
static void music_kill(void) {
    system("killall madplay 2>/dev/null");
}

/* ===== 播放当前歌曲（从头开始） ===== */
static void music_play_current(void) {
    if (!g_playlist || g_count <= 0) return;
    music_kill();

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "madplay \"%s\" &", g_playlist[g_index]);
    system(cmd);
    g_playing = 1;

    gettimeofday(&g_play_start, NULL);
    g_duration_sec = get_mp3_duration(g_playlist[g_index]);

    if (g_slider) lv_slider_set_value(g_slider, 0, LV_ANIM_OFF);
    update_cover();

    printf("[音乐] 播放 %s (第 %d/%d 首, 时长 %d 秒)\n",
           g_playlist[g_index], g_index + 1, g_count, g_duration_sec);
}

/* ===== 进度条定时器 ===== */
static void progress_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (g_count <= 0) return;

    if (g_user_seeking) return;

    if (g_grace_ticks > 0) g_grace_ticks--;

    if (!g_playing || g_duration_sec <= 0) return;

    struct timeval now;
    gettimeofday(&now, NULL);
    float elapsed = (now.tv_sec - g_play_start.tv_sec)
                  + (now.tv_usec - g_play_start.tv_usec) / 1000000.0f;

    /* 按累计时间判断播放结束，不用 pidof */
    if (g_grace_ticks == 0 && elapsed >= g_duration_sec + 1) {
        printf("[音乐] 播放结束，自动下一首\n");
        g_index = (g_index + 1) % g_count;
        g_grace_ticks = 4;
        music_play_current();
        return;
    }

    if (!g_slider) return;
    int percent = (int)(elapsed * 100 / g_duration_sec);
    if (percent > 100) percent = 100;
    if (percent < 0)   percent = 0;
    lv_slider_set_value(g_slider, percent, LV_ANIM_OFF);
}

/* ===== 滑块拖动 ===== */
static void slider_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        g_user_seeking = 1;
        printf("[音乐] 开始拖动进度条\n");
    } else if (code == LV_EVENT_RELEASED) {
        int percent = lv_slider_get_value(g_slider);
        int seek_sec = g_duration_sec * percent / 100;

        /* 限幅：不超过 duration-5，防止 seek 到末尾立即结束 */
        if (seek_sec > g_duration_sec - 5) seek_sec = g_duration_sec - 5;
        if (seek_sec < 0) seek_sec = 0;

        printf("[音乐] 跳转到 %d%% (%d 秒 / 总 %d 秒)\n",
               percent, seek_sec, g_duration_sec);

        if (g_playlist && g_count > 0) {
            music_kill();

            char cmd[300];
            snprintf(cmd, sizeof(cmd),
                     "madplay --start=%d \"%s\" &",
                     seek_sec, g_playlist[g_index]);
            system(cmd);

            /* 强制进入播放状态（拖动即播放） */
            gettimeofday(&g_play_start, NULL);
            g_play_start.tv_sec -= seek_sec;
            g_playing = 1;

            g_grace_ticks = 6;
        }
        g_user_seeking = 0;
    }
}

static void slider_delete_cb(lv_event_t *e) {
    if (lv_event_get_target(e) == g_slider) g_slider = NULL;
}

/* ===== 播放/暂停 ===== */
static void play_toggle_cb(lv_event_t *e) {
    (void)e;
    if (g_count <= 0) return;
    if (g_playing) {
        system("killall -STOP madplay 2>/dev/null");
        g_playing = 0;
        printf("[音乐] 暂停\n");
    } else {
        int alive = (system("pidof madplay > /dev/null 2>&1") == 0);
        if (alive) {
            system("killall -CONT madplay 2>/dev/null");
            g_playing = 1;
            printf("[音乐] 恢复\n");
        } else {
            music_play_current();
        }
    }
}

/* ===== 下一首 ===== */
static void next_song_cb(lv_event_t *e) {
    (void)e;
    if (g_count <= 0) return;
    g_index = (g_index + 1) % g_count;
    g_grace_ticks = 4;
    music_play_current();
}

/* ===== 上一首 ===== */
static void prev_song_cb(lv_event_t *e) {
    (void)e;
    if (g_count <= 0) return;
    g_index = (g_index - 1 + g_count) % g_count;
    g_grace_ticks = 4;
    music_play_current();
}

/* ===== 初始化 ===== */
void ui_music_init(lv_obj_t *play_icon,
                   lv_obj_t *prev_icon,
                   lv_obj_t *next_icon,
                   const char **playlist,
                   const char **cover_list,
                   int count)
{
    if (!playlist || count <= 0) return;

    g_playlist   = playlist;
    g_cover_list = cover_list;
    g_count      = count;
    g_index      = 0;
    g_playing    = 0;

    if (play_icon) lv_obj_add_event_cb(play_icon, play_toggle_cb, LV_EVENT_CLICKED, NULL);
    if (prev_icon) lv_obj_add_event_cb(prev_icon, prev_song_cb,   LV_EVENT_CLICKED, NULL);
    if (next_icon) lv_obj_add_event_cb(next_icon, next_song_cb,   LV_EVENT_CLICKED, NULL);

    g_progress_timer = lv_timer_create(progress_timer_cb, 500, NULL);
    if (g_progress_timer) lv_timer_set_repeat_count(g_progress_timer, -1);

    update_cover();
    printf("[音乐] 初始化完成，共 %d 首歌\n", count);
}

void ui_music_set_cover(lv_obj_t *cover_img) {
    g_cover_img = cover_img;
    update_cover();
    printf("[音乐] 封面控件已绑定\n");
}

void ui_music_set_slider(lv_obj_t *slider) {
    if (!slider) return;
    g_slider = slider;
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb,  LV_EVENT_PRESSED,  NULL);
    lv_obj_add_event_cb(slider, slider_event_cb,  LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(slider, slider_delete_cb, LV_EVENT_DELETE,   NULL);
    printf("[音乐] 进度条已绑定\n");
}

void ui_music_cleanup(void) {
    music_kill();
    if (g_progress_timer) {
        lv_timer_del(g_progress_timer);
        g_progress_timer = NULL;
    }
    g_playing = 0;
}