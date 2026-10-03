#include "flappy_audio.h"
#include "flappy_ui.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static const char *TAG = "voice_flappy";
static QueueHandle_t s_keys;
static bool s_nvs_ready;
typedef struct { bsp_btn_t button; bsp_btn_ev_t event; } key_event_t;
static void button_callback(bsp_btn_t button, bsp_btn_ev_t event, void *user) {
    (void)user;
    if (event != BSP_BTN_PRESS && !(event == BSP_BTN_LONG && button == BSP_BTN_OK)) return;
    key_event_t key = {button, event};
    (void)xQueueSend(s_keys, &key, 0);
}
static void load_records(fb_game_t *g) {
    nvs_handle_t h;
    if (!s_nvs_ready || nvs_open("voice_flappy", NVS_READONLY, &h) != ESP_OK) return;
    uint8_t value;
    (void)nvs_get_u16(h, "best", &g->best);
    if (nvs_get_u8(h, "sensitivity", &value) == ESP_OK && value <= 2) g->sensitivity = value;
    if (nvs_get_u8(h, "sound", &value) == ESP_OK) g->sound = value != 0;
    if (nvs_get_u8(h, "practice", &value) == ESP_OK) g->practice = value != 0;
    nvs_close(h);
}
static void save_records(const fb_game_t *g) {
    nvs_handle_t h;
    if (!s_nvs_ready || nvs_open("voice_flappy", NVS_READWRITE, &h) != ESP_OK) return;
    esp_err_t e = nvs_set_u16(h, "best", g->best);
    if (e == ESP_OK) e = nvs_set_u8(h, "sensitivity", g->sensitivity);
    if (e == ESP_OK) e = nvs_set_u8(h, "sound", g->sound);
    if (e == ESP_OK) e = nvs_set_u8(h, "practice", g->practice);
    if (e == ESP_OK) e = nvs_commit(h);
    if (e != ESP_OK) ESP_LOGW(TAG, "save failed: %s", esp_err_to_name(e));
    nvs_close(h);
}
static void game_task(void *arg) {
    (void)arg;
    fb_game_t game; fb_init(&game, esp_random()); load_records(&game);
    TickType_t wake = xTaskGetTickCount();
    uint32_t sequence = 0, frames = 0;
    int battery = -1;
    bool dirty = false;
    int64_t persist_at = 0;
    for (;;) {
        int64_t now = esp_timer_get_time()/1000;
        key_event_t key;
        while (xQueueReceive(s_keys, &key, 0) == pdTRUE) {
            uint8_t sensitivity = game.sensitivity;
            bool sound = game.sound, practice = game.practice;
            fb_key(&game, key.event == BSP_BTN_LONG ? FB_BACK : key.button == BSP_BTN_OK ? FB_OK : key.button == BSP_BTN_UP ? FB_UP : FB_DOWN);
            if (sensitivity != game.sensitivity || sound != game.sound || practice != game.practice) {
                dirty = true; persist_at = now + 1000;
            }
        }
        fb_audio_sample_t audio = fb_audio_snapshot();
        bool fresh = audio.sequence != sequence;
        sequence = audio.sequence;
        bool ready = audio.ready && now-audio.sampled_ms < 400;
        if (!ready) fb_mic_failed(&game);
        uint16_t best = game.best;
        fb_event_t event = fb_step(&game, audio.level, fresh && ready && audio.usable);
        if (game.sound && event != FB_SILENT) fb_audio_cue(event);
        if (best != game.best) { dirty = true; persist_at = now; }
        /* Storage and shared-I2C reads occur outside callbacks/LVGL lock. */
        if (dirty && now >= persist_at && game.state != FB_PLAY && game.state != FB_COUNTDOWN && game.state != FB_CALIBRATE) {
            save_records(&game); dirty = false;
        }
        if (frames % 500 == 0) {
            battery = bsp_battery_soc();
            ESP_LOGI(TAG, "state=%d heap=%lu largest=%lu game_stack=%lu", game.state,
                (unsigned long)esp_get_free_heap_size(),
                (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                (unsigned long)uxTaskGetStackHighWaterMark(NULL));
        }
        if (frames++ % 2 == 0 && bsp_lvgl_lock(20)) {
            fb_ui_render(&game, battery, ready); bsp_lvgl_unlock();
        }
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(FB_STEP_MS));
        /* Drop excessive catch-up after slow storage instead of a physics burst. */
        if ((int32_t)(xTaskGetTickCount()-wake) > pdMS_TO_TICKS(100)) wake = xTaskGetTickCount();
    }
}
void app_main(void) {
    esp_err_t e = nvs_flash_init();
    /* Preserve unrelated NVS contents even if the partition needs recovery. */
    s_nvs_ready = e == ESP_OK;
    if (!s_nvs_ready) ESP_LOGW(TAG, "NVS unavailable: %s", esp_err_to_name(e));
    ESP_ERROR_CHECK(bsp_display_init());
    if (!bsp_lvgl_init()) { ESP_LOGE(TAG, "LVGL init failed"); return; }
    s_keys = xQueueCreate(12, sizeof(key_event_t));
    if (!s_keys) { ESP_LOGE(TAG, "input queue allocation failed"); return; }
    if (!bsp_lvgl_lock(1000)) return;
    bool ui_ok = fb_ui_create(); bsp_lvgl_unlock();
    if (!ui_ok) { ESP_LOGE(TAG, "font coverage failed"); return; }
    bsp_display_backlight(85);
    e = bsp_battery_init();
    if (e != ESP_OK) ESP_LOGW(TAG, "battery unavailable");
    e = fb_audio_start();
    if (e != ESP_OK) ESP_LOGW(TAG, "audio worker unavailable: %s", esp_err_to_name(e));
    ESP_ERROR_CHECK(bsp_button_init(button_callback, NULL));
    if (xTaskCreate(game_task, "voice_game", 4096, NULL, 3, NULL) != pdPASS)
        ESP_LOGE(TAG, "game task allocation failed");
    ESP_LOGI(TAG, "Voice Flappy started, free heap=%lu", (unsigned long)esp_get_free_heap_size());
}
