#pragma once
#include "flappy_model.h"
#include <stdbool.h>
bool fb_ui_create(void);
void fb_ui_render(const fb_game_t *game, int battery, bool mic_ready);
