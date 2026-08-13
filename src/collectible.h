#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H

#include <stdbool.h>

#include "game_state.h"

void collectibleInitialize(void);
void collectibleHide(void);
bool collectibleSpawn(const GameData *state);
bool collectibleTouchesHead(const GameData *state, float headRadius);
void collectibleUpdatePhysics(const GameData *state);
void collectiblePushFromBody(const GameData *state);
void collectibleDraw(const GameData *state, float rotation);

#endif
