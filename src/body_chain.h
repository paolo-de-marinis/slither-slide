#ifndef BODY_CHAIN_H
#define BODY_CHAIN_H

#include "game_state.h"

void bodyChainInitialize(GameData *state);
void bodyChainGrow(GameData *state);
void bodyChainUpdate(GameData *state);

#endif
