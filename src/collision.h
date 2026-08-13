#ifndef COLLISION_H
#define COLLISION_H

#include <stdbool.h>

#include "game_state.h"

float collisionHeadRadius(void);
bool collisionAtNextHead(const GameData *state, riv_vec2i nextHeadPosition);

#endif
