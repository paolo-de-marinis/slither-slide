#include "levels.h"

#include "audio.h"
#include "collectible.h"
#include "game_state.h"
#include "scoring.h"
#include "walls.h"

#include <stddef.h>
#include <string.h>

static const LevelConfig levelConfigs[MAX_LEVEL] = {{5, 6, 1, 100},
                                                    {8, 6, 1, 200},
                                                    {10, 5, 1, 300},
                                                    {12, 5, 1, 400},
                                                    {15, 5, 1, 500},
                                                    {18, 4, 1, 600},
                                                    {20, 4, 1, 700},
                                                    {25, 4, 1, 1000},
                                                    {30, 3, 1, 1500},
                                                    {35, 3, 1, 2000},
                                                    {40, 3, 1, 2500},
                                                    {50, 3, 1, 5000}};

static bool buildLevelWalls(int level);
static void addLevelObstacles(int level, float roomOffsetX, float roomOffsetY);
static void openAvailableDoors(void);

void resetLevelProgress(void) {
    game.currentLevel = 1;
    memset(game.levelItems, 0, sizeof(game.levelItems));
    memset(game.levelCompleted, 0, sizeof(game.levelCompleted));
}

bool getLevelConfig(int level, LevelConfig *config) {
    if (level < 1 || level > MAX_LEVEL || config == NULL) {
        return false;
    }
    *config = levelConfigs[level - 1];
    return true;
}

int getCurrentLevel(void) {
    return game.currentLevel;
}

bool isLevelCompleted(int level) {
    return level >= 1 && level <= MAX_LEVEL && game.levelCompleted[level - 1];
}

bool isLevelComplete(void) {
    LevelConfig config;
    return getLevelConfig(game.currentLevel, &config) &&
           (isLevelCompleted(game.currentLevel) ||
            game.levelItems[game.currentLevel - 1] >= config.requiredItems);
}

bool canTransitionToLevel(int level) {
    return canTraverseLevels(game.currentLevel, level, isLevelComplete());
}

bool initializeLevel(int levelNumber) {
    LevelConfig config;
    if (!getLevelConfig(levelNumber, &config)) {
        return false;
    }

    game.currentLevel = levelNumber;
    game.moveDelay = config.moveDelay;
    game.growthRate = config.growthRate;
    if (!buildLevelWalls(levelNumber)) {
        return false;
    }

    scoringBeginLevel(&game, game.currentLevel);
    if (isLevelCompleted(game.currentLevel)) {
        collectibleHide();
        return true;
    }
    return collectibleSpawn(&game);
}

bool checkDoorState(void) {
    LevelConfig config;
    if (!getLevelConfig(game.currentLevel, &config) || isLevelCompleted(game.currentLevel) ||
        game.levelItems[game.currentLevel - 1] < config.requiredItems) {
        return false;
    }

    scoringCompleteLevel(&game, game.currentLevel, config.bonusPoints);
    collectibleHide();
    openAvailableDoors();
    playDoorSound();
    return true;
}

bool handleLevelTransition(void) {
    float headX = game.headPosition.x * TILE_SIZE + TILE_SIZE / 2.0f;
    float headY = game.headPosition.y * TILE_SIZE + TILE_SIZE / 2.0f;
    int roomX = (int)(headX / ROOM_WIDTH);
    int roomY = (int)(headY / ROOM_HEIGHT);
    int nextLevel = getLevelAtPosition(roomX, roomY);

    if (nextLevel == 0 || nextLevel == game.currentLevel) {
        return true;
    }
    if (!canTransitionToLevel(nextLevel)) {
        return false;
    }

    scoringPauseLevel(&game);
    return initializeLevel(nextLevel);
}

static bool buildLevelWalls(int level) {
    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(level, &roomX, &roomY)) {
        return false;
    }

    float roomOffsetX = roomX * ROOM_WIDTH;
    float roomOffsetY = roomY * ROOM_HEIGHT;
    wallsBeginRoom(roomOffsetX, roomOffsetY);
    openAvailableDoors();
    addLevelObstacles(level, roomOffsetX, roomOffsetY);
    return true;
}

static void openAvailableDoors(void) {
    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(game.currentLevel, &roomX, &roomY)) {
        return;
    }

    const int offsetX[WALL_SIDE_COUNT] = {-1, 1, 0, 0};
    const int offsetY[WALL_SIDE_COUNT] = {0, 0, -1, 1};

    for (int side = 0; side < WALL_SIDE_COUNT; side++) {
        int neighbor = getLevelAtPosition(roomX + offsetX[side], roomY + offsetY[side]);
        if (!canTransitionToLevel(neighbor)) {
            continue;
        }
        for (int segment = 0; segment < DOOR_SEGMENT_COUNT; segment++) {
            wallsRemoveBoundarySegment((WallSide)side, DOOR_START_SEGMENT + segment);
        }
    }
}

static void addLevelObstacles(int level, float roomOffsetX, float roomOffsetY) {
    switch (level) {
        case 2:
            wallsAdd((Wall){roomOffsetX + ROOM_WIDTH / 2 - 20,
                            roomOffsetY + ROOM_HEIGHT / 2 - 20,
                            40,
                            40});
            break;
        case 3:
            wallsAdd((Wall){roomOffsetX + ROOM_WIDTH / 3 - 20,
                            roomOffsetY + ROOM_HEIGHT / 3 - 20,
                            40,
                            40});
            wallsAdd((Wall){roomOffsetX + 2 * ROOM_WIDTH / 3 - 20,
                            roomOffsetY + 2 * ROOM_HEIGHT / 3 - 20,
                            40,
                            40});
            break;
        case 4:
            wallsAdd((Wall){roomOffsetX + ROOM_WIDTH / 2 - 60,
                            roomOffsetY + ROOM_HEIGHT / 2 - 10,
                            120,
                            20});
            wallsAdd((Wall){roomOffsetX + ROOM_WIDTH / 2 - 10,
                            roomOffsetY + ROOM_HEIGHT / 2 - 60,
                            20,
                            120});
            break;
        case 5:
            for (int obstacle = 0; obstacle < 3; obstacle++) {
                wallsAdd((Wall){roomOffsetX + (obstacle + 1) * ROOM_WIDTH / 4 - 10,
                                roomOffsetY + ROOM_HEIGHT / 4,
                                20,
                                ROOM_HEIGHT / 2});
            }
            break;
        default:
            break;
    }
}
