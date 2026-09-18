SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen2.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_try_png.c
    images/ui_img_zsp1_png.c
    images/ui_img_zz_png.c
    images/ui_img_shache_png.c
    images/ui_img_youzxd_png.c
    images/ui_img_youmen_png.c
    images/ui_img_zzxd_png.c
    images/ui_img_zhuangshi6_png.c
    images/ui_img_zhuangshi4_png.c
    images/ui_img_zhuangshi5_png.c
    images/ui_img_zanting_png.c
    images/ui_img_qianyishou_png.c
    images/ui_img_img11_png.c
    fonts/ui_font_chinese.c)

add_library(ui ${SOURCES})
