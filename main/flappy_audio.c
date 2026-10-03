#include "flappy_audio.h"
#include "bsp_audio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define FRAME 320
static QueueHandle_t s_cues;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static fb_audio_sample_t s_sample;
static const char *TAG = "voice_audio";
fb_audio_sample_t fb_audio_snapshot(void) {
    portENTER_CRITICAL(&s_mux);
    fb_audio_sample_t sample = s_sample;
    portEXIT_CRITICAL(&s_mux);
    return sample;
}
void fb_audio_cue(fb_event_t cue) {
    if (s_cues) (void)xQueueSend(s_cues, &cue, 0);
}
static void audio_task(void *arg) {
    (void)arg;
    int16_t input[FRAME], output[FRAME];
    bool ready = false;
    uint32_t phase = 0, played = 0, duration = 0;
    int64_t guard_until = 0;
    fb_event_t cue = FB_SILENT;
    for (;;) {
        if (!ready) {
            esp_err_t e = bsp_audio_init();
            if (e == ESP_OK) e = bsp_audio_wake();
            if (e == ESP_OK) e = bsp_audio_set_format(16000, 16, 1);
            if (e != ESP_OK) {
                ESP_LOGW(TAG, "audio retry: %s", esp_err_to_name(e));
                vTaskDelay(pdMS_TO_TICKS(2000)); continue;
            }
            bsp_audio_set_volume(40); ready = true;
        }
        esp_err_t e = bsp_audio_read(input, sizeof(input));
        int64_t now = esp_timer_get_time() / 1000;
        fb_event_t next;
        if (xQueueReceive(s_cues, &next, 0) == pdTRUE) {
            cue = next; played = 0; phase = 0;
            duration = cue == FB_CRASH ? 3200 : cue == FB_START ? 1280 : 640;
        }
        bool audible = played < duration;
        if (audible) guard_until = now + 120;
        portENTER_CRITICAL(&s_mux);
        s_sample.ready = e == ESP_OK;
        s_sample.usable = e == ESP_OK && !audible && now >= guard_until;
        s_sample.level = e == ESP_OK ? fb_pcm_level(input, FRAME) : 0;
        s_sample.sampled_ms = now; s_sample.sequence++;
        portEXIT_CRITICAL(&s_mux);
        for (unsigned i = 0; i < FRAME; ++i) {
            int32_t value = 0;
            if (played < duration) {
                unsigned hz = cue == FB_CRASH ? 460 - 300 * played / duration :
                              cue == FB_START ? (played < duration/2 ? 660 : 880) :
                              (played < duration/2 ? 880 : 1175);
                phase = (phase + hz * 65536u / 16000u) & 65535u;
                int32_t triangle = phase < 32768 ? (int32_t)phase - 16384 : 49152 - (int32_t)phase;
                unsigned envelope = duration - played < 160 ? duration - played : played < 80 ? played * 2 : 160;
                value = triangle * (int32_t)envelope / 1800;
                played++;
            }
            output[i] = (int16_t)value;
        }
        if (e == ESP_OK) e = bsp_audio_write(output, sizeof(output));
        if (e != ESP_OK) {
            portENTER_CRITICAL(&s_mux); s_sample.ready = false; portEXIT_CRITICAL(&s_mux);
            ESP_LOGW(TAG, "PCM failed: %s", esp_err_to_name(e));
            /* This worker is the sole owner; PCM has stopped before suspend. */
            (void)bsp_audio_sleep(); ready = false; duration = played = 0;
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
esp_err_t fb_audio_start(void) {
    s_cues = xQueueCreate(4, sizeof(fb_event_t));
    if (!s_cues) return ESP_ERR_NO_MEM;
    if (xTaskCreate(audio_task, "voice_pcm", 4096, NULL, 4, NULL) != pdPASS) {
        vQueueDelete(s_cues); s_cues = NULL; return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
