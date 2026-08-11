#ifndef GAME_H
#define GAME_H

#include "riv.h"
#include "objects.h"
#include "char_selector.h"
#include "snake_char.h"
#include "caterpillar_char.h"
#include "joint_point.h"
#include "levels.h"
#include "camera.h"
#include "room_layout.h"

// Game States
typedef enum {
    GAME_STATE_CHAR_SELECT,
    GAME_STATE_PLAYING,
    GAME_STATE_OVER,
    GAME_STATE_COMPLETED
} GameState;

// Game Constants
enum { WALL_THICKNESS = 4, MAP_SIZE = 42, TILE_SIZE = 6 };

#define MOVE_DELAY_DEFAULT 6 // 10 moves per second at 60fps
#define TARGET_FPS 60

#define GROWTH_RATE 1
#define WORLD_TILES_X ((WORLD_WIDTH + TILE_SIZE - 1) / TILE_SIZE)
#define WORLD_TILES_Y ((WORLD_HEIGHT + TILE_SIZE - 1) / TILE_SIZE)

#define DEBUG_MODE 0 // Set to 1 for diagnostics, geometry overlays and R3 level skip.

// Physics constants
#define PHYSICS_FRICTION 0.95f
#define COLLISION_FORCE 1.0f
#define MIN_VELOCITY 0.1f
#define NORMAL_EPSILON 0.0001f

// Game state variables
typedef struct {
    GameState state;
    bool started;
    bool ended;
    int apples;
    int ticks;
    int score;
    riv_vec2i headPosition;
    riv_vec2i headDirection;
    riv_vec2i tailPosition;
    riv_vec2i snakeBody[WORLD_TILES_Y][WORLD_TILES_X];
    float appleRotation;

    // Character animation states
    SnakeAnimationState snakeAnimation;

    // Joint management
    JointPoint joints[MAX_JOINTS];
    int jointCount;

    // Movement control
    int moveTimer;
    int moveDelay;

    // Level-related fields
    int applesRequired;
    int growthRate;
    int levelScores[MAX_LEVEL];  // Array to store scores for each level
    int currentLevelScore;       // Score for current level only
    int totalScore;              // Cumulative score across all levels
    float levelTimes[MAX_LEVEL]; // Time spent in each level in seconds
    float currentLevelStartTime; // When the current level started

} GameData;

// Core game functions
void gameInitialize(void);
void gameUpdate(void);
void gameDraw(void);
void gameStart(void);
void gameEnd(void);
void gameComplete(void);

#endif
