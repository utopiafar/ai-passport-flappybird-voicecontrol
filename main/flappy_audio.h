#pragma once
#include "flappy_model.h"
#include "esp_err.h"
typedef struct {
    uint16_t level;
    uint32_t sequence;
    int64_t sampled_ms;
    bool ready, usable;
} fb_audio_sample_t;
esp_err_t fb_audio_start(void);
fb_audio_sample_t fb_audio_snapshot(void);
void fb_audio_cue(fb_event_t cue);
