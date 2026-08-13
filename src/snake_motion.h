#ifndef SNAKE_MOTION_H
#define SNAKE_MOTION_H

#include <stdbool.h>

#include "game_state.h"

bool snakeMotionUpdate(GameData *state);
bool snakeCollectItem(GameData *state);

#endif
