#include "game.h"
#include "levels.h"
#include "objects.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

GameData game;
ObjectState object;

enum {
    LEFT_SIDE,
    RIGHT_SIDE,
    TOP_SIDE,
    BOTTOM_SIDE
};

bool spawnApple(void) {
    return true;
}

void playDoorSound(void) {
}

static bool isDoorOpen(int side) {
    for (int segment = 0; segment < DOOR_SEGMENT_COUNT; segment++) {
        int wallIndex = side * WALL_SEGMENTS_PER_SIDE +
                        DOOR_START_SEGMENT + segment;
        if (object.walls[wallIndex].width != 0.0f ||
            object.walls[wallIndex].height != 0.0f) {
            return false;
        }
    }
    return true;
}

static void completeCurrentLevel(void) {
    game.apples = getLevelConfig(getCurrentLevel()).requiredApples;
    checkDoorState();
}

static riv_vec2i tileInsideLevel(int level) {
    int roomX = 0;
    int roomY = 0;
    assert(getRoomPosition(level, &roomX, &roomY));
    return (riv_vec2i){
        roomX * ROOM_WIDTH / TILE_SIZE + 2,
        roomY * ROOM_HEIGHT / TILE_SIZE + 2
    };
}

static void testVerticalTransition(int firstLevel, int firstSide,
                                   int secondLevel, int secondSide) {
    initializeLevel(firstLevel);
    assert(!isDoorOpen(firstSide));
    completeCurrentLevel();
    assert(isDoorOpen(firstSide));

    game.headPosition = tileInsideLevel(secondLevel);
    handleLevelTransition();
    assert(getCurrentLevel() == secondLevel);
    assert(isDoorOpen(secondSide));

    game.headPosition = tileInsideLevel(firstLevel);
    handleLevelTransition();
    assert(getCurrentLevel() == firstLevel);
}

int main(void) {
    resetLevelProgress();

    testVerticalTransition(2, BOTTOM_SIDE, 3, TOP_SIDE);
    testVerticalTransition(4, BOTTOM_SIDE, 5, TOP_SIDE);
    testVerticalTransition(8, TOP_SIDE, 9, BOTTOM_SIDE);
    testVerticalTransition(11, BOTTOM_SIDE, 12, TOP_SIDE);

    puts("vertical doors: ok");
    return 0;
}
