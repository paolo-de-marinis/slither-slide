#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

#include "joint_point.h"
#include "levels.h"
#include "riv.h"
#include "room_layout.h"
#include "snake_char.h"

typedef enum {
    GAME_STATE_CHAR_SELECT,
    GAME_STATE_PLAYING,
    GAME_STATE_OVER,
    GAME_STATE_COMPLETED
} GameState;

enum {
    WALL_THICKNESS = 4,
    MAP_SIZE = 42,
    TILE_SIZE = 6,
    INITIAL_SNAKE_LENGTH = 20,
    MOVE_DELAY_DEFAULT = 6,
    TARGET_FPS = 60,
    GROWTH_RATE_DEFAULT = 1
};

#define WORLD_TILES_X ((WORLD_WIDTH + TILE_SIZE - 1) / TILE_SIZE)
#define WORLD_TILES_Y ((WORLD_HEIGHT + TILE_SIZE - 1) / TILE_SIZE)

#ifndef DEBUG_MODE
#define DEBUG_MODE 0
#endif

#ifndef CHEATS_ENABLED
#define CHEATS_ENABLED 0
#endif

typedef struct {
    GameState state;
    int ticks;
    int score;

    riv_vec2i headPosition;
    riv_vec2i headDirection;
    riv_vec2i tailPosition;
    riv_vec2i bodyDirections[WORLD_TILES_Y][WORLD_TILES_X];

    JointPoint joints[MAX_JOINTS];
    int jointCount;
    SnakeAnimationState snakeAnimation;

    int moveTimer;
    int moveDelay;
    int growthRate;
    float collectibleRotation;

    int levelItems[MAX_LEVEL];
    int levelScores[MAX_LEVEL];
    int levelElapsedTicks[MAX_LEVEL];
    bool levelCompleted[MAX_LEVEL];
    int currentLevel;
    int levelEntryTick;
    bool levelTimerRunning;
} GameData;

extern GameData game;

#endif
