/* Host-only visual QA using the same LVGL UI and font as the firmware.
 * See render_flappy_ui.sh; this never opens a USB device. */
#include "flappy_ui.h"
#include "lvgl.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t pixels[240*320*3];
static uint8_t buffer[240*320*3];
static uint32_t tick;
static uint32_t ticks(void) { return tick; }
static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data) {
    int width=area->x2-area->x1+1;
    for(int y=area->y1;y<=area->y2;y++) {
        memcpy(pixels+(y*240+area->x1)*3, data+(y-area->y1)*width*3, width*3);
    }
    lv_display_flush_ready(display);
}
static void snapshot(fb_game_t *g, const char *directory, const char *name) {
    fb_ui_render(g,78,true); tick+=40; lv_refr_now(NULL);
    /* The ground must be present even behind overlays; catches an unbound or
     * uninitialized canvas, which compilation/glyph checks cannot detect. */
    unsigned ground=(254*240+10)*3;
    assert(pixels[ground] == 0xA0 && pixels[ground+1] == 0xD9 && pixels[ground+2] == 0xEB);
    char path[1024]; snprintf(path,sizeof(path),"%s/%s.ppm",directory,name);
    FILE *f=fopen(path,"wb"); assert(f);
    fputs("P6\n240 320\n255\n",f);
    for(unsigned i=0;i<sizeof(pixels);i+=3) {
        /* LVGL RGB888 byte order is B, G, R. */
        fputc(pixels[i+2],f);fputc(pixels[i+1],f);fputc(pixels[i],f);
    }
    fclose(f);
}
int main(int argc,char **argv) {
    assert(argc==2); lv_init(); lv_tick_set_cb(ticks);
    lv_display_t *d=lv_display_create(240,320);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB888);
    lv_display_set_buffers(d,buffer,NULL,sizeof(buffer),LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(d,flush);
    assert(fb_ui_create());
    fb_game_t g;fb_init(&g,42);snapshot(&g,argv[1],"home");
    g.state=FB_CALIBRATE;g.cal_frames=30;snapshot(&g,argv[1],"calibration");
    g.state=FB_COUNTDOWN;g.clock_ms=1200;snapshot(&g,argv[1],"countdown");
    g.state=FB_PLAY;g.score=12;g.voice=.7f;g.pipes[0].x=160;snapshot(&g,argv[1],"flight");
    g.state=FB_PAUSE;snapshot(&g,argv[1],"pause");
    g.state=FB_RESULT;g.score=30;g.best=30;snapshot(&g,argv[1],"result");
    g.state=FB_MIC_ERROR;snapshot(&g,argv[1],"mic-error");
    g.noisy=true;snapshot(&g,argv[1],"noise-error");
    g.state=FB_HOME;g.practice=true;g.sound=false;snapshot(&g,argv[1],"practice");
    puts("Actual LVGL UI host rendering: PASS");return 0;
}
