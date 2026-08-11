#include "room_layout.h"

#include <stdlib.h>

const int LEVEL_MATRIX[ROOMS_Y][ROOMS_X] = {
    {1, 2, 11, 10},
    {4, 3, 12, 9},
    {5, 6, 7, 8}
};

bool getRoomPosition(int level, int *roomX, int *roomY) {
    if (level <= 0 || roomX == NULL || roomY == NULL) {
        return false;
    }

    for (int y = 0; y < ROOMS_Y; y++) {
        for (int x = 0; x < ROOMS_X; x++) {
            if (LEVEL_MATRIX[y][x] == level) {
                *roomX = x;
                *roomY = y;
                return true;
            }
        }
    }

    return false;
}

int getLevelAtPosition(int roomX, int roomY) {
    if (roomX < 0 || roomX >= ROOMS_X || roomY < 0 || roomY >= ROOMS_Y) {
        return 0;
    }

    return LEVEL_MATRIX[roomY][roomX];
}

bool areLevelsAdjacent(int firstLevel, int secondLevel) {
    int firstX = 0;
    int firstY = 0;
    int secondX = 0;
    int secondY = 0;

    if (!getRoomPosition(firstLevel, &firstX, &firstY) ||
        !getRoomPosition(secondLevel, &secondX, &secondY)) {
        return false;
    }

    return abs(firstX - secondX) + abs(firstY - secondY) == 1;
}

bool canTraverseLevels(int firstLevel, int secondLevel, bool firstCompleted) {
    if (!areLevelsAdjacent(firstLevel, secondLevel)) {
        return false;
    }

    if (secondLevel == firstLevel - 1) {
        return true;
    }

    return secondLevel == firstLevel + 1 && firstCompleted;
}
