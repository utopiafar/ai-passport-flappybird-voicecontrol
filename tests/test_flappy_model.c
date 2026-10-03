#include "flappy_model.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
static void start(fb_game_t *g) {
    fb_init(g, 123); fb_key(g, FB_OK);
    for (int i=0;i<60;i++) fb_step(g, 100, true);
    assert(g->state == FB_COUNTDOWN && g->noise == 100);
    for (int i=0;i<150;i++) fb_step(g, 0, true);
    assert(g->state == FB_PLAY);
}
int main(void) {
    int16_t dc[] = {20000,20000,20000,20000}, pcm[] = {-32768,32767};
    assert(fb_pcm_level(dc,4)==0 && fb_pcm_level(pcm,2)==32767);
    assert(fb_pcm_level(NULL,0)==0);
    assert(fb_voice_target(100,100,1)==0 && fb_voice_target(32000,100,1)==1);
    assert(fb_voice_target(1000,100,2)>fb_voice_target(1000,100,0));
    fb_game_t soft, loud; start(&soft); start(&loud);
    for(int i=0;i<10;i++) { fb_step(&soft,1100,true); fb_step(&loud,3000,true); }
    assert(loud.y<soft.y && loud.velocity<soft.velocity);
    fb_game_t g; start(&g);
    float initial=g.y;
    for(int i=0;i<12;i++) fb_step(&g,2200,true);
    assert(g.y<initial && g.velocity<0);
    fb_key(&g,FB_OK); assert(g.state==FB_PAUSE);
    float paused=g.y, x=g.pipes[0].x;
    for(int i=0;i<50;i++) fb_step(&g,30000,true);
    assert(g.y==paused && g.pipes[0].x==x);
    fb_key(&g,FB_UP); assert(g.sensitivity==2);
    fb_key(&g,FB_OK); assert(g.state==FB_COUNTDOWN);
    for(int i=0;i<50;i++) fb_step(&g,0,true);
    assert(g.state==FB_PLAY && g.y==paused);
    fb_key(&g,FB_BACK); assert(g.state==FB_HOME);
    start(&g);
    for(int i=0;i<200 && g.state==FB_PLAY;i++) fb_step(&g,0,true);
    assert(g.state==FB_RESULT); // silence reaches floor
    start(&g); g.pipes[0].x=FB_BIRD_X; g.pipes[0].gap_y=45;
    assert(fb_step(&g,0,true)==FB_CRASH);
    start(&g); g.y=1; assert(fb_step(&g,0,true)==FB_CRASH);
    start(&g); g.pipes[0].x=FB_BIRD_X-FB_RADIUS-FB_PIPE_WIDTH-1;
    assert(fb_step(&g,0,true)==FB_POINT && g.score==1);
    fb_step(&g,0,true); assert(g.score==1);
    g.y=FB_FLOOR; fb_step(&g,0,true); assert(g.best==1);
    start(&g); g.practice=true; g.score=20; g.y=FB_FLOOR;
    fb_step(&g,0,true); assert(g.best==0 && fb_gap(&g)==104);
    fb_init(&g,0); fb_key(&g,FB_OK);
    for(int i=0;i<60;i++) fb_step(&g,12000,true);
    assert(g.state==FB_MIC_ERROR && g.noisy);
    fb_key(&g,FB_OK);
    for(int i=0;i<201;i++) fb_step(&g,0,false);
    assert(g.state==FB_MIC_ERROR); // suppressed frames cannot calibrate
    start(&g); fb_step(&g,2000,true);
    float voice=g.voice; fb_step(&g,32767,false); assert(g.voice==voice);
    fb_mic_failed(&g); assert(g.state==FB_MIC_ERROR);
    fb_key(&g,FB_BACK); fb_mic_failed(&g); assert(g.state==FB_HOME);
    start(&g); g.pipes[0].x=-40;
    fb_step(&g,700,true); assert(g.pipes[0].x>g.pipes[2].x && !g.pipes[0].scored);
    for(int i=0;i<100;i++) fb_key(&g,FB_UP);
    assert(g.sensitivity==2);
    for(int i=0;i<100;i++) fb_key(&g,FB_DOWN);
    assert(g.sensitivity==0);
    puts("Voice Flappy model tests: PASS"); return 0;
}
