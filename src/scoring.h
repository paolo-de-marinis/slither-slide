#ifndef SCORING_H
#define SCORING_H

#include "game_state.h"

void scoringInitialize(GameData *state);
void scoringBeginLevel(GameData *state, int level);
void scoringPauseLevel(GameData *state);
void scoringCompleteLevel(GameData *state, int level, int bonusPoints);
void scoringUpdate(GameData *state);
void scoringRefreshOutcard(GameData *state);

#endif
