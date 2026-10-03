#ifndef LV_CONF_H
#define LV_CONF_H
/* Match the firmware's constrained LVGL heap for actual UI QA on the host. */
#define LV_COLOR_DEPTH 16
#define LV_MEM_SIZE (24 * 1024)
#define LV_USE_CANVAS 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_USE_FONT_PLACEHOLDER 1
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1
#endif
