#include "game_state.h"
#include "levels.h"
#include "scoring.h"
#include "walls.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

GameData game;

static int spawnCount;
static int hideCount;
static int completedScoreCount;

bool collectibleSpawn(const GameData *state) {
    (void)state;
    spawnCount++;
    return true;
}

void collectibleHide(void) {
    hideCount++;
}

void playDoorSound(void) {
}

void scoringBeginLevel(GameData *state, int level) {
    assert(state->currentLevel == level);
}

void scoringPauseLevel(GameData *state) {
    (void)state;
}

void scoringCompleteLevel(GameData *state, int level, int bonusPoints) {
    state->levelScores[level - 1] += bonusPoints;
    state->levelCompleted[level - 1] = true;
    completedScoreCount++;
}

void worldToScreen(float worldX, float worldY, float *screenX, float *screenY) {
    *screenX = worldX;
    *screenY = worldY;
}

void riv_draw_rect_fill(int64_t x, int64_t y, int64_t width, int64_t height, uint32_t color) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}

static bool isDoorOpen(WallSide side) {
    for (int segment = 0; segment < DOOR_SEGMENT_COUNT; segment++) {
        int wallIndex = side * WALL_SEGMENTS_PER_SIDE + DOOR_START_SEGMENT + segment;
        const Wall *wall = wallsGet(wallIndex);
        if (wall == NULL || wall->width != 0.0f || wall->height != 0.0f) {
            return false;
        }
    }
    return true;
}

static riv_vec2i tileInsideLevel(int level) {
    int roomX = 0;
    int roomY = 0;
    assert(getRoomPosition(level, &roomX, &roomY));
    return (riv_vec2i){roomX * ROOM_WIDTH / TILE_SIZE + 2,
                       roomY * ROOM_HEIGHT / TILE_SIZE + 2};
}

static WallSide sideFromDelta(int deltaX, int deltaY) {
    if (deltaX < 0) {
        return WALL_SIDE_LEFT;
    }
    if (deltaX > 0) {
        return WALL_SIDE_RIGHT;
    }
    return deltaY < 0 ? WALL_SIDE_TOP : WALL_SIDE_BOTTOM;
}

static WallSide oppositeSide(WallSide side) {
    const WallSide opposite[WALL_SIDE_COUNT] = {
        WALL_SIDE_RIGHT,
        WALL_SIDE_LEFT,
        WALL_SIDE_BOTTOM,
        WALL_SIDE_TOP
    };
    return opposite[side];
}

int main(void) {
    LevelConfig invalid;
    assert(!getLevelConfig(0, &invalid));
    assert(!getLevelConfig(MAX_LEVEL + 1, &invalid));
    assert(!getLevelConfig(1, NULL));

    resetLevelProgress();
    assert(initializeLevel(1));

    for (int level = 1; level < MAX_LEVEL; level++) {
        int roomX = 0;
        int roomY = 0;
        int nextRoomX = 0;
        int nextRoomY = 0;
        assert(getRoomPosition(level, &roomX, &roomY));
        assert(getRoomPosition(level + 1, &nextRoomX, &nextRoomY));
        WallSide forwardSide = sideFromDelta(nextRoomX - roomX, nextRoomY - roomY);

        assert(!isDoorOpen(forwardSide));
        LevelConfig config;
        assert(getLevelConfig(level, &config));
        game.levelItems[level - 1] = config.requiredItems;
        assert(checkDoorState());
        assert(isDoorOpen(forwardSide));

        game.headPosition = tileInsideLevel(level + 1);
        assert(handleLevelTransition());
        assert(getCurrentLevel() == level + 1);
        assert(isDoorOpen(oppositeSide(forwardSide)));
    }

    assert(completedScoreCount == MAX_LEVEL - 1);

    /* A completed room has no collectible and does not erase later progress. */
    game.levelItems[MAX_LEVEL - 1] = 7;
    int spawnsBeforeBacktrack = spawnCount;
    game.headPosition = tileInsideLevel(MAX_LEVEL - 1);
    assert(handleLevelTransition());
    assert(getCurrentLevel() == MAX_LEVEL - 1);
    assert(spawnCount == spawnsBeforeBacktrack);
    assert(hideCount > 0);
    assert(game.levelItems[MAX_LEVEL - 1] == 7);

    puts("level doors and backtracking: ok");
    return 0;
}
