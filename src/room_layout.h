#ifndef ROOM_LAYOUT_H
#define ROOM_LAYOUT_H

#include <stdbool.h>

#define ROOMS_X 4
#define ROOMS_Y 3
#define ROOM_WIDTH 256
#define ROOM_HEIGHT 256
#define WORLD_WIDTH (ROOMS_X * ROOM_WIDTH)
#define WORLD_HEIGHT (ROOMS_Y * ROOM_HEIGHT)

/* Room-space topology: each entry names one 256 x 256 region of the global world. */
extern const int LEVEL_MATRIX[ROOMS_Y][ROOMS_X];

bool getRoomPosition(int level, int *roomX, int *roomY);
int getLevelAtPosition(int roomX, int roomY);
bool areLevelsAdjacent(int firstLevel, int secondLevel);
bool canTraverseLevels(int firstLevel, int secondLevel, bool firstCompleted);

#endif
