#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Run after the IDF build has downloaded the pinned LVGL managed component.
mkdir -p build/ui-preview
cmake -S managed_components/lvgl__lvgl -B build/host-lvgl \
    -DCONFIG_LV_BUILD_EXAMPLES=OFF -DCONFIG_LV_BUILD_DEMOS=OFF \
    -DLV_BUILD_CONF_PATH="$PWD/tools/lv_conf_host.h" \
    -DCMAKE_C_FLAGS=''
cmake --build build/host-lvgl --parallel 8
cc -std=c11 "-DLV_CONF_PATH=\"$PWD/tools/lv_conf_host.h\"" \
    -Imain -Imanaged_components/lvgl__lvgl \
    tools/render_flappy_ui.c main/flappy_ui.c main/flappy_model.c \
    assets/fonts/flappy_font_16.c assets/fonts/flappy_glyphs.c \
    build/host-lvgl/lib/liblvgl.a -lm -o build/ui-preview/render
build/ui-preview/render build/ui-preview
