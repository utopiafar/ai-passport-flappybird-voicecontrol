idf_component_register(
    SRCS "main.c" "flappy_model.c" "flappy_audio.c" "flappy_ui.c"
         "../assets/fonts/flappy_font_16.c" "../assets/fonts/flappy_glyphs.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer esp_hw_support nvs_flash
)
