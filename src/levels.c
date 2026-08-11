#include "levels.h"

#include "audio.h"
#include "game.h"
#include "objects.h"

#include <string.h>

extern GameData game;

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

static int currentLevel = 1;
static int highestCompletedLevel = 0;
static bool doorOpen = false;
static bool bonusAwarded[MAX_LEVEL];

static void clearLevelWalls(void);
static void addBoundaryWalls(float roomOffsetX, float roomOffsetY);
static void addLevelObstacles(int level, float roomOffsetX, float roomOffsetY);
static void removeWallSegment(int side, int segment);
static void openAvailableDoors(void);

void resetLevelProgress(void) {
    currentLevel = 1;
    highestCompletedLevel = 0;
    doorOpen = false;
    memset(bonusAwarded, 0, sizeof(bonusAwarded));
}

void initializeLevel(int levelNumber) {
    int roomX = 0;
    int roomY = 0;

    if (levelNumber < 1 || levelNumber > MAX_LEVEL ||
        !getRoomPosition(levelNumber, &roomX, &roomY)) {
        return;
    }

    currentLevel = levelNumber;
    LevelConfig config = levelConfigs[currentLevel - 1];
    game.moveDelay = config.moveDelay;
    game.applesRequired = config.requiredApples;
    game.growthRate = config.growthRate;
    game.apples = 0;
    game.currentLevelStartTime = game.ticks / (float)TARGET_FPS;
    doorOpen = currentLevel <= highestCompletedLevel;

    addLevelWalls(currentLevel);
    spawnApple();
}

int getCurrentLevel(void) {
    return currentLevel;
}

LevelConfig getLevelConfig(int level) {
    if (level < 1 || level > MAX_LEVEL) {
        return levelConfigs[0];
    }

    return levelConfigs[level - 1];
}

bool isLevelComplete(void) {
    return currentLevel <= highestCompletedLevel ||
           game.apples >= levelConfigs[currentLevel - 1].requiredApples;
}

bool canTransitionToLevel(int level) {
    return canTraverseLevels(currentLevel, level, isLevelComplete());
}

void checkDoorState(void) {
    if (doorOpen || game.apples < levelConfigs[currentLevel - 1].requiredApples) {
        return;
    }

    doorOpen = true;
    if (currentLevel > highestCompletedLevel) {
        highestCompletedLevel = currentLevel;
    }
    openAvailableDoors();
    playDoorSound();
}

void handleLevelTransition(void) {
    float headX = game.headPosition.x * TILE_SIZE + TILE_SIZE / 2.0f;
    float headY = game.headPosition.y * TILE_SIZE + TILE_SIZE / 2.0f;
    int roomX = (int)(headX / ROOM_WIDTH);
    int roomY = (int)(headY / ROOM_HEIGHT);
    int nextLevel = getLevelAtPosition(roomX, roomY);

    if (nextLevel == 0 || nextLevel == currentLevel || !canTransitionToLevel(nextLevel)) {
        return;
    }

    int previousLevel = currentLevel;
    if (nextLevel == previousLevel + 1 && !bonusAwarded[previousLevel - 1]) {
        game.score += levelConfigs[previousLevel - 1].bonusPoints;
        bonusAwarded[previousLevel - 1] = true;
    }

    initializeLevel(nextLevel);
}

static void removeWallSegment(int side, int segment) {
    if (side < 0 || side >= 4 || segment < 0 || segment >= WALL_SEGMENTS_PER_SIDE) {
        return;
    }

    int wallIndex = side * WALL_SEGMENTS_PER_SIDE + segment;
    object.walls[wallIndex].width = 0.0f;
    object.walls[wallIndex].height = 0.0f;
}

static void openAvailableDoors(void) {
    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(currentLevel, &roomX, &roomY)) {
        return;
    }

    const int offsetX[] = {-1, 1, 0, 0};
    const int offsetY[] = {0, 0, -1, 1};
    const int wallSide[] = {0, 1, 2, 3};

    for (int direction = 0; direction < 4; direction++) {
        int neighbor = getLevelAtPosition(roomX + offsetX[direction], roomY + offsetY[direction]);
        if (!canTransitionToLevel(neighbor)) {
            continue;
        }

        for (int segment = 0; segment < DOOR_SEGMENT_COUNT; segment++) {
            removeWallSegment(wallSide[direction], DOOR_START_SEGMENT + segment);
        }
    }
}

void addLevelWalls(int level) {
    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(level, &roomX, &roomY)) {
        return;
    }

    clearLevelWalls();
    float roomOffsetX = roomX * ROOM_WIDTH;
    float roomOffsetY = roomY * ROOM_HEIGHT;
    addBoundaryWalls(roomOffsetX, roomOffsetY);
    openAvailableDoors();
    addLevelObstacles(level, roomOffsetX, roomOffsetY);
}

static void addBoundaryWalls(float roomOffsetX, float roomOffsetY) {
    float segmentWidth = ROOM_WIDTH / (float)WALL_SEGMENTS_PER_SIDE;
    float segmentHeight = ROOM_HEIGHT / (float)WALL_SEGMENTS_PER_SIDE;

    for (int segment = 0; segment < WALL_SEGMENTS_PER_SIDE; segment++) {
        object.walls[segment] = (Wall){roomOffsetX - WALL_THICKNESS,
                                       roomOffsetY + segment * segmentHeight,
                                       WALL_THICKNESS * 2,
                                       segmentHeight};
        object.walls[WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){roomOffsetX + ROOM_WIDTH - WALL_THICKNESS,
                   roomOffsetY + segment * segmentHeight,
                   WALL_THICKNESS * 2,
                   segmentHeight};
        object.walls[2 * WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){roomOffsetX + segment * segmentWidth, roomOffsetY, segmentWidth, WALL_THICKNESS};
        object.walls[3 * WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){roomOffsetX + segment * segmentWidth,
                   roomOffsetY + ROOM_HEIGHT - WALL_THICKNESS,
                   segmentWidth,
                   WALL_THICKNESS};
    }

    object.wallCount = WALL_SEGMENTS_PER_SIDE * 4;
}

static void addLevelObstacles(int level, float roomOffsetX, float roomOffsetY) {
    switch (level) {
        case 2:
            object.walls[object.wallCount++] = (Wall){
                roomOffsetX + ROOM_WIDTH / 2 - 20, roomOffsetY + ROOM_HEIGHT / 2 - 20, 40, 40};
            break;
        case 3:
            object.walls[object.wallCount++] = (Wall){
                roomOffsetX + ROOM_WIDTH / 3 - 20, roomOffsetY + ROOM_HEIGHT / 3 - 20, 40, 40};
            object.walls[object.wallCount++] = (Wall){roomOffsetX + 2 * ROOM_WIDTH / 3 - 20,
                                                      roomOffsetY + 2 * ROOM_HEIGHT / 3 - 20,
                                                      40,
                                                      40};
            break;
        case 4:
            object.walls[object.wallCount++] = (Wall){
                roomOffsetX + ROOM_WIDTH / 2 - 60, roomOffsetY + ROOM_HEIGHT / 2 - 10, 120, 20};
            object.walls[object.wallCount++] = (Wall){
                roomOffsetX + ROOM_WIDTH / 2 - 10, roomOffsetY + ROOM_HEIGHT / 2 - 60, 20, 120};
            break;
        case 5:
            for (int obstacle = 0; obstacle < 3; obstacle++) {
                object.walls[object.wallCount++] =
                    (Wall){roomOffsetX + (obstacle + 1) * ROOM_WIDTH / 4 - 10,
                           roomOffsetY + ROOM_HEIGHT / 4,
                           20,
                           ROOM_HEIGHT / 2};
            }
            break;
        default:
            break;
    }
}

static void clearLevelWalls(void) {
    object.wallCount = 0;
}
