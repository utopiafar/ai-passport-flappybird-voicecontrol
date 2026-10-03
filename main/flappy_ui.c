#include "flappy_ui.h"
#include "lvgl.h"
#include <string.h>
LV_FONT_DECLARE(flappy_font_16);
LV_DRAW_BUF_DEFINE_STATIC(scene_buf, FB_WIDTH, FB_HEIGHT, LV_COLOR_FORMAT_I4);
static lv_obj_t *s_screen, *s_canvas, *s_score, *s_battery, *s_meter, *s_status;
static lv_obj_t *s_panel, *s_title, *s_body, *s_action;
static const uint32_t palette[16] = {
    0x80D8DF, 0xDBF4E6, 0xAEDBCF, 0x487864,
    0x254C43, 0x6DC653, 0xA9E66C, 0x368943,
    0xEBD9A0, 0xD0B774, 0xFFF0A3, 0xF2B943,
    0xE67341, 0xFFFFFF, 0x142F37, 0xB8EAE7,
};
static lv_obj_t *label(lv_obj_t *parent, int x, int y, int w, const char *text) {
    lv_obj_t *o = lv_label_create(parent);
    lv_obj_set_pos(o, x, y); lv_obj_set_width(o, w);
    lv_obj_set_style_text_font(o, &flappy_font_16, 0);
    lv_obj_set_style_text_color(o, lv_color_hex(0x254C43), 0);
    lv_label_set_text(o, text); return o;
}
/* Palette rasterizer writes the actual LVGL draw buffer, respecting stride.
 * Clip once per rectangle; rows use packed nibbles instead of 52k API calls. */
static void rect(int x, int y, int w, int h, unsigned color) {
    int right = x+w, bottom = y+h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right > FB_WIDTH) right = FB_WIDTH;
    if (bottom > FB_HEIGHT) bottom = FB_HEIGHT;
    if (x >= right || y >= bottom) return;
    uint8_t *pixels = scene_buf.data + 16 * 4;
    for (int row = y; row < bottom; ++row) {
        uint8_t *p = pixels + row * scene_buf.header.stride;
        int first = x, end = right;
        if (first & 1) { p[first/2] = (p[first/2] & 0xF0) | color; first++; }
        if (end & 1) { end--; p[end/2] = (p[end/2] & 0x0F) | (color << 4); }
        if (end > first) memset(p + first/2, color * 17, (end-first)/2);
    }
}
static void pipe(int x, int y, int height, bool upper) {
    if (height <= 0) return;
    rect(x, y, FB_PIPE_WIDTH, height, 4);
    rect(x+3, y, FB_PIPE_WIDTH-6, height, 5);
    rect(x+5, y, 5, height, 6);
    rect(x+FB_PIPE_WIDTH-8, y, 5, height, 7);
    int lip = upper ? y+height-10 : y;
    rect(x, lip, FB_PIPE_WIDTH, 10, 4);
    rect(x+2, lip+2, FB_PIPE_WIDTH-4, 6, 5);
    rect(x+4, lip+2, 6, 6, 6);
}
static void bird(int y, bool wing) {
    static const char *sprite[] = {
        "....EEEEEE......", "..EEAAAAAAEE....", ".EAAAAAAAAADE...",
        "EAAABBAAAADDEE..", "EAABBBBAAADDEE..", "EBBBBBBAAADEEE..",
        "EBBBBBBAACCCCCCE", ".EBBBBAAACCCCCCE", "..EAAAAAACCCCCE.",
        "...EEAAAAAAEEE..", ".....EEEEEE.....",
    };
    for (unsigned row = 0; row < 11; ++row)
        for (unsigned col = 0; col < 16; ++col) {
            char c = sprite[row][col];
            if (c != '.') rect(FB_BIRD_X-12+(int)col*2, y-10+(int)row*2, 2, 2, c-'A'+10);
        }
    if (wing) { rect(FB_BIRD_X-8, y-2, 10, 3, 11); rect(FB_BIRD_X-6, y-5, 6, 3, 10); }
}
static bool font_check(void) {
    extern const uint32_t flappy_codepoints[];
    extern const unsigned flappy_codepoint_count;
    for (unsigned i = 0; i < flappy_codepoint_count; ++i) {
        lv_font_glyph_dsc_t glyph = {0};
        if (!lv_font_get_glyph_dsc(&flappy_font_16, &glyph, flappy_codepoints[i], 0) || glyph.is_placeholder)
            return false;
    }
    lv_font_glyph_dsc_t missing = {0};
    return !lv_font_get_glyph_dsc(&flappy_font_16, &missing, 0x9F98, 0) || missing.is_placeholder;
}
bool fb_ui_create(void) {
    if (!font_check()) return false;
    s_screen = lv_obj_create(NULL); lv_obj_remove_style_all(s_screen);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0xDBF4E6), 0);
    lv_obj_set_style_bg_opa(s_screen, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    label(s_screen, 24, 13, 80, "声翼");
    s_score = label(s_screen, 98, 13, 64, "0");
    lv_obj_set_style_text_align(s_score, LV_TEXT_ALIGN_CENTER, 0);
    s_battery = label(s_screen, 168, 13, 48, "--%");
    lv_obj_set_style_text_align(s_battery, LV_TEXT_ALIGN_RIGHT, 0);
    LV_DRAW_BUF_INIT_STATIC(scene_buf);
    s_canvas = lv_canvas_create(s_screen); lv_canvas_set_draw_buf(s_canvas, &scene_buf);
    lv_obj_set_pos(s_canvas, 0, 40);
    for (int i = 0; i < 16; ++i) lv_canvas_set_palette(s_canvas, i, lv_color32_make(palette[i]>>16, palette[i]>>8, palette[i], 255));
    s_status = label(s_screen, 18, 267, 204, "声音越大飞得越高");
    lv_obj_set_style_text_align(s_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *track = lv_obj_create(s_screen); lv_obj_remove_style_all(track);
    lv_obj_set_pos(track, 26, 293); lv_obj_set_size(track, 188, 7);
    lv_obj_set_style_bg_color(track, lv_color_hex(0xAEDBCF), 0); lv_obj_set_style_bg_opa(track, 255, 0);
    s_meter = lv_obj_create(track); lv_obj_remove_style_all(s_meter);
    lv_obj_set_size(s_meter, 1, 7);
    lv_obj_set_style_bg_color(s_meter, lv_color_hex(0x368943), 0); lv_obj_set_style_bg_opa(s_meter, 255, 0);
    s_panel = lv_obj_create(s_screen); lv_obj_remove_style_all(s_panel);
    lv_obj_set_pos(s_panel, 16, 62); lv_obj_set_size(s_panel, 208, 178);
    lv_obj_set_style_bg_color(s_panel, lv_color_hex(0xFFF0D0), 0); lv_obj_set_style_bg_opa(s_panel, 255, 0);
    lv_obj_set_style_border_width(s_panel, 3, 0); lv_obj_set_style_border_color(s_panel, lv_color_hex(0x254C43), 0);
    lv_obj_set_style_radius(s_panel, 8, 0); lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    s_title = label(s_panel, 6, 10, 190, "声控飞行");
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    s_body = label(s_panel, 12, 40, 178, ""); lv_obj_set_style_text_align(s_body, LV_TEXT_ALIGN_CENTER, 0);
    s_action = label(s_panel, 8, 144, 186, "确定开始"); lv_obj_set_style_text_align(s_action, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_action, lv_color_hex(0x368943), 0);
    lv_screen_load(s_screen); return true;
}
void fb_ui_render(const fb_game_t *g, int battery, bool mic_ready) {
    static uint32_t frame;
    ++frame;
    rect(0, 0, FB_WIDTH, FB_HEIGHT, 0);
    int cloud = 200 - (int)(frame % 340);
    rect(cloud, 25, 36, 8, 1); rect(cloud+8, 19, 18, 6, 1);
    rect(cloud+110, 62, 44, 8, 1); rect(cloud+120, 54, 20, 8, 1);
    for (int x = 0; x < FB_WIDTH; x += 24) {
        rect(x, 174-(x%3)*6, 18, 30+(x%3)*6, 2); rect(x+3, 182, 3, 5, 1);
    }
    for (int i = 0; i < FB_PIPE_COUNT; ++i) {
        int top = g->pipes[i].gap_y - fb_gap(g)/2, bottom = top + fb_gap(g);
        pipe((int)g->pipes[i].x, 0, top, true);
        pipe((int)g->pipes[i].x, bottom, FB_FLOOR-bottom, false);
    }
    rect(0, FB_FLOOR, FB_WIDTH, 3, 4); rect(0, FB_FLOOR+3, FB_WIDTH, 4, 6);
    rect(0, FB_FLOOR+7, FB_WIDTH, 13, 8);
    for (int x = -(int)(frame%16); x < FB_WIDTH; x+=16) rect(x, FB_FLOOR+12, 7, 2, 9);
    bird((int)g->y, (frame/3)%2 == 0); lv_obj_invalidate(s_canvas);
    lv_label_set_text_fmt(s_score, "%u", g->score);
    if (battery < 0) lv_label_set_text(s_battery, "--%");
    else lv_label_set_text_fmt(s_battery, "%d%%", battery);
    lv_obj_set_width(s_meter, 1+(int)(g->voice*187));
    const char *sensitivity[] = {"低", "中", "高"};
    if (g->state == FB_PLAY || g->state == FB_PAUSE)
        lv_label_set_text_fmt(s_status, "灵敏度%s  上加下减", sensitivity[g->sensitivity]);
    else lv_label_set_text(s_status, mic_ready ? "声音越大飞得越高" : "麦克风正在连接");
    if (g->state == FB_PLAY) { lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN); return; }
    lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_HIDDEN);
    switch (g->state) {
        case FB_HOME:
            lv_label_set_text(s_title, "声控飞行");
            lv_label_set_text_fmt(s_body, "%s\n上键音效%s\n下键切换模式\n最高 %u", g->practice ? "宽管练习" : "经典挑战", g->sound ? "开" : "关", g->best);
            lv_label_set_text(s_action, "确定开始"); break;
        case FB_CALIBRATE:
            lv_label_set_text(s_title, "环境校准");
            lv_label_set_text(s_body, "请保持安静\n远离扬声器\n采集环境音量");
            lv_label_set_text_fmt(s_action, "%u%%", (unsigned)(g->cal_frames*100/60)); break;
        case FB_COUNTDOWN:
            lv_label_set_text(s_title, "准备飞行");
            lv_label_set_text_fmt(s_body, "%u\n发声上升\n安静下落", (unsigned)((g->resumed ? 1000 : 3000)-g->clock_ms+999)/1000);
            lv_label_set_text(s_action, "确定暂停"); break;
        case FB_PAUSE:
            lv_label_set_text(s_title, "已暂停");
            lv_label_set_text(s_body, "上键增加灵敏度\n下键降低灵敏度\n长按确定回首页");
            lv_label_set_text(s_action, "确定继续"); break;
        case FB_RESULT:
            lv_label_set_text(s_title, g->practice ? "练习结束" : g->score >= 30 ? "金牌飞行" : g->score >= 15 ? "银牌飞行" : g->score >= 5 ? "铜牌飞行" : "再试一次");
            lv_label_set_text_fmt(s_body, "本次 %u\n最高 %u\n长按确定回首页", g->score, g->best);
            lv_label_set_text(s_action, "确定再来"); break;
        case FB_MIC_ERROR:
            lv_label_set_text(s_title, g->noisy ? "环境太吵" : "麦克风不可用");
            lv_label_set_text(s_body, g->noisy ? "换个安静的位置\n校准时请勿发声\n长按确定回首页" : "请稍后重试\n长按确定回首页");
            lv_label_set_text(s_action, "确定重试"); break;
        default: break;
    }
}
