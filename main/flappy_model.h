#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define FB_WIDTH 240
#define FB_HEIGHT 220
#define FB_FLOOR 204
#define FB_BIRD_X 58
#define FB_RADIUS 8
#define FB_PIPE_WIDTH 34
#define FB_PIPE_COUNT 3
#define FB_STEP_MS 20

typedef enum { FB_HOME, FB_CALIBRATE, FB_COUNTDOWN, FB_PLAY, FB_PAUSE, FB_RESULT, FB_MIC_ERROR } fb_state_t;
typedef enum { FB_OK, FB_UP, FB_DOWN, FB_BACK } fb_key_t;
typedef enum { FB_SILENT = 0, FB_START = 1, FB_POINT = 2, FB_CRASH = 4 } fb_event_t;
typedef struct { float x; int gap_y; bool scored; } fb_pipe_t;
typedef struct {
    fb_state_t state;
    fb_pipe_t pipes[FB_PIPE_COUNT];
    float y, velocity, voice;
    uint32_t rng, clock_ms, noise_sum, cal_frames;
    uint16_t noise, score, best;
    uint8_t sensitivity;
    bool practice, sound, resumed, noisy;
} fb_game_t;

void fb_init(fb_game_t *g, uint32_t seed);
void fb_key(fb_game_t *g, fb_key_t key);
/* One fixed 20 ms step. valid=false means speaker suppression; no calibration
 * sample or voice update occurs. The caller separately rejects stale audio. */
fb_event_t fb_step(fb_game_t *g, uint16_t level, bool valid);
void fb_mic_failed(fb_game_t *g);
uint16_t fb_pcm_level(const int16_t *samples, size_t count);
float fb_voice_target(uint16_t level, uint16_t noise, unsigned sensitivity);
int fb_gap(const fb_game_t *g);
