#ifndef LEVELS_H
#define LEVELS_H

#include <stdbool.h>

#include "room_layout.h"

#define MAX_LEVEL 12
#define DOOR_START_SEGMENT 3
#define DOOR_SEGMENT_COUNT 2

typedef struct {
    int requiredItems;
    int moveDelay;
    int growthRate;
    int bonusPoints;
} LevelConfig;

void resetLevelProgress(void);
bool initializeLevel(int levelNumber);
int getCurrentLevel(void);
bool getLevelConfig(int level, LevelConfig *config);
bool isLevelCompleted(int level);
bool isLevelComplete(void);
bool canTransitionToLevel(int level);
bool checkDoorState(void);
bool handleLevelTransition(void);

#endif
