#include "flappy_model.h"
#include <string.h>

static uint32_t random_next(fb_game_t *g) {
    g->rng ^= g->rng << 13; g->rng ^= g->rng >> 17; g->rng ^= g->rng << 5;
    return g->rng;
}
int fb_gap(const fb_game_t *g) { return g->practice ? 104 : 84; }
static int next_gap(fb_game_t *g) {
    int half = fb_gap(g) / 2;
    return half + 18 + (int)(random_next(g) % (FB_FLOOR - 2 * half - 36));
}
static void reset_run(fb_game_t *g) {
    g->y = 102; g->velocity = 0; g->voice = 0; g->score = 0;
    for (int i = 0; i < FB_PIPE_COUNT; ++i)
        g->pipes[i] = (fb_pipe_t){300.0f + i * 148.0f, next_gap(g), false};
}
void fb_init(fb_game_t *g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->state = FB_HOME; g->rng = seed ? seed : 1; g->noise = 80;
    g->sensitivity = 1; g->sound = true; reset_run(g);
}
void fb_key(fb_game_t *g, fb_key_t key) {
    if (key == FB_BACK) { g->state = FB_HOME; g->voice = 0; return; }
    if (g->state == FB_HOME) {
        if (key == FB_UP) g->sound = !g->sound;
        if (key == FB_DOWN) g->practice = !g->practice;
    } else if (g->state == FB_PLAY || g->state == FB_PAUSE) {
        if (key == FB_UP && g->sensitivity < 2) g->sensitivity++;
        if (key == FB_DOWN && g->sensitivity > 0) g->sensitivity--;
    }
    if (key != FB_OK) return;
    switch (g->state) {
        case FB_HOME: case FB_RESULT: case FB_MIC_ERROR:
            reset_run(g); g->cal_frames = 0; g->noise_sum = 0;
            g->clock_ms = 0; g->noisy = false; g->state = FB_CALIBRATE; break;
        case FB_PLAY: g->state = FB_PAUSE; break;
        case FB_PAUSE:
            g->state = FB_COUNTDOWN; g->clock_ms = 0; g->resumed = true;
            g->voice = 0; break;
        default: break;
    }
}
uint16_t fb_pcm_level(const int16_t *samples, size_t count) {
    if (!samples || !count || count > 4096) return 0;
    int64_t sum = 0;
    for (size_t i = 0; i < count; ++i) sum += samples[i];
    int32_t mean = (int32_t)(sum / (int64_t)count);
    uint64_t energy = 0;
    for (size_t i = 0; i < count; ++i) {
        int32_t v = (int32_t)samples[i] - mean;
        energy += (uint32_t)(v < 0 ? -v : v);
    }
    uint64_t result = energy / count;
    return (uint16_t)(result > 32768 ? 32768 : result);
}
float fb_voice_target(uint16_t level, uint16_t noise, unsigned sensitivity) {
    static const float span[] = {3200, 1900, 1100};
    if (sensitivity > 2) sensitivity = 2;
    float threshold = noise * 1.8f + 120;
    float value = (level - threshold) / (span[sensitivity] + noise * 0.6f);
    return value < 0 ? 0 : value > 1 ? 1 : value;
}
void fb_mic_failed(fb_game_t *g) {
    if (g->state == FB_PLAY || g->state == FB_CALIBRATE || g->state == FB_COUNTDOWN)
        g->state = FB_MIC_ERROR;
}
fb_event_t fb_step(fb_game_t *g, uint16_t level, bool valid) {
    if (g->state == FB_CALIBRATE) {
        g->clock_ms += FB_STEP_MS;
        if (valid) { g->noise_sum += level; g->cal_frames++; }
        if (g->cal_frames >= 60) {
            uint32_t noise = g->noise_sum / g->cal_frames;
            if (noise > 7000) { g->noisy = true; g->state = FB_MIC_ERROR; return FB_SILENT; }
            g->noise = noise < 50 ? 50 : (uint16_t)noise;
            g->clock_ms = 0; g->resumed = false; g->state = FB_COUNTDOWN;
        } else if (g->clock_ms > 4000) g->state = FB_MIC_ERROR;
        return FB_SILENT;
    }
    if (g->state == FB_COUNTDOWN) {
        g->clock_ms += FB_STEP_MS;
        if (g->clock_ms >= (g->resumed ? 1000u : 3000u)) {
            g->state = FB_PLAY; return FB_START;
        }
        return FB_SILENT;
    }
    if (g->state != FB_PLAY) return FB_SILENT;
    if (valid) {
        float target = fb_voice_target(level, g->noise, g->sensitivity);
        g->voice += (target - g->voice) * (target > g->voice ? 0.45f : 0.18f);
    }
    /* Sustained loudness sets a proportional climb rate. Silence restores
     * gravity; no repeated edge trigger and no button-controlled lift. */
    if (g->voice > 0.03f) {
        float target_v = 65.0f - 230.0f * g->voice;
        g->velocity += (target_v - g->velocity) * 0.28f;
    } else g->velocity += 8.0f;
    if (g->velocity > 185) g->velocity = 185;
    g->y += g->velocity * 0.02f;
    bool collision = g->y - FB_RADIUS <= 0 || g->y + FB_RADIUS >= FB_FLOOR;
    fb_event_t event = FB_SILENT;
    float speed = g->practice ? 62 : 76 + (g->score < 30 ? g->score : 30) * 0.7f;
    for (int i = 0; i < FB_PIPE_COUNT; ++i) {
        fb_pipe_t *p = &g->pipes[i]; p->x -= speed * 0.02f;
        if (p->x < -FB_PIPE_WIDTH) {
            float right = 0;
            for (int j = 0; j < FB_PIPE_COUNT; ++j) if (g->pipes[j].x > right) right = g->pipes[j].x;
            p->x = right + 148; p->gap_y = next_gap(g); p->scored = false;
        }
        if (p->x < FB_BIRD_X + FB_RADIUS && p->x + FB_PIPE_WIDTH > FB_BIRD_X - FB_RADIUS &&
            (g->y - FB_RADIUS < p->gap_y - fb_gap(g)/2 || g->y + FB_RADIUS > p->gap_y + fb_gap(g)/2))
            collision = true;
        if (!p->scored && p->x + FB_PIPE_WIDTH < FB_BIRD_X - FB_RADIUS) {
            p->scored = true;
            if (g->score < UINT16_MAX) g->score++;
            event = FB_POINT;
        }
    }
    if (collision) {
        g->state = FB_RESULT;
        if (!g->practice && g->score > g->best) g->best = g->score;
        return FB_CRASH;
    }
    return event;
}
