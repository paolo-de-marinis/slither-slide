#ifndef LEVELS_H
#define LEVELS_H
#include <stdbool.h>
#include "room_layout.h"

#define MAX_LEVEL 12

#define DOOR_START_SEGMENT 3  // Starting segment for the door
#define DOOR_SEGMENT_COUNT 2  // Number of segments that make up the door

typedef struct {
    int requiredApples;
    int moveDelay;
    int growthRate;
    int bonusPoints;
} LevelConfig;

void initializeLevel(int levelNumber);
int getCurrentLevel(void);
LevelConfig getLevelConfig(int level);
bool isLevelComplete(void);
void handleLevelTransition(void);
void checkDoorState(void);
void addLevelWalls(int level);
bool canTransitionToLevel(int level);
void resetLevelProgress(void);

#endif // LEVELS_H
