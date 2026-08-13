#include "game.h"

#include "audio.h"
#include "body_chain.h"
#include "camera.h"
#include "char_selector.h"
#include "collectible.h"
#include "controls.h"
#include "levels.h"
#include "scoring.h"
#include "snake_motion.h"
#include "walls.h"

#include <string.h>

GameData game;

#if CHEATS_ENABLED
static bool handleCheatLevelSkip(void);
#endif

void gameInitialize(void) {
    memset(&game, 0, sizeof(game));
    collectibleInitialize();
    wallsInitializeMenu();
    cameraInitialize();
    resetLevelProgress();

    game.state = GAME_STATE_CHAR_SELECT;
    game.moveDelay = MOVE_DELAY_DEFAULT;
    game.growthRate = GROWTH_RATE_DEFAULT;
    initializeSnakeAnimation(&game.snakeAnimation);
    initializeSkinSelector();
    scoringInitialize(&game);
    startBackgroundMusic();
}

void gameStart(void) {
    if (DEBUG_MODE) {
        riv_printf("Game started\n");
    }

    game.state = GAME_STATE_PLAYING;
    game.ticks = 0;
    game.moveTimer = 0;
    game.collectibleRotation = 0.0f;
    memset(game.bodyDirections, 0, sizeof(game.bodyDirections));

    game.headDirection = (riv_vec2i){0, -1};
    game.headPosition = (riv_vec2i){MAP_SIZE / 4, MAP_SIZE / 2};
    game.tailPosition =
        (riv_vec2i){game.headPosition.x, game.headPosition.y + INITIAL_SNAKE_LENGTH - 1};
    for (int index = 0; index < INITIAL_SNAKE_LENGTH; index++) {
        game.bodyDirections[game.headPosition.y + index][game.headPosition.x] =
            game.headDirection;
    }

    bodyChainInitialize(&game);
    collectibleInitialize();
    resetLevelProgress();
    cameraInitialize();
    scoringInitialize(&game);
    if (!initializeLevel(1)) {
        gameEnd();
        return;
    }
    playStartSound();
}

#if CHEATS_ENABLED
static bool handleCheatLevelSkip(void) {
    if (!riv->keys[CONTROL_CHEAT_NEXT_LEVEL].press) {
        return true;
    }

    int level = getCurrentLevel();
    LevelConfig config;
    if (!getLevelConfig(level, &config)) {
        gameEnd();
        return false;
    }

    game.levelItems[level - 1] = config.requiredItems;
    checkDoorState();
    if (level == MAX_LEVEL) {
        gameComplete();
        return false;
    }
    if (!initializeLevel(level + 1)) {
        gameEnd();
        return false;
    }
    riv_printf("Cheat: skipped to level %d\n", level + 1);
    return true;
}
#endif

void gameUpdate(void) {
    if (game.state == GAME_STATE_CHAR_SELECT) {
        updateSkinSelection();
        if (isSkinSelected()) {
            gameStart();
        }
        return;
    }
    if (game.state != GAME_STATE_PLAYING) {
        return;
    }

    game.ticks++;
#if CHEATS_ENABLED
    if (!handleCheatLevelSkip()) {
        return;
    }
#endif
    if (getCurrentSkin() == SKIN_SNAKE) {
        updateSnakeAnimation(&game.snakeAnimation);
    }
    if (!snakeMotionUpdate(&game)) {
        return;
    }

    cameraUpdate();
    bodyChainUpdate(&game);
    scoringUpdate(&game);
    collectibleUpdatePhysics(&game);
    collectiblePushFromBody(&game);
}

void gameEnd(void) {
    if (game.state == GAME_STATE_OVER || game.state == GAME_STATE_COMPLETED) {
        return;
    }
    if (DEBUG_MODE) {
        riv_printf("Game Over at frame %d: score=%d, length=%d\n",
                   riv->frame,
                   game.score,
                   game.jointCount);
    }

    scoringPauseLevel(&game);
    game.state = GAME_STATE_OVER;
    riv->quit_frame = riv->frame + 3 * riv->target_fps;
    stopBackgroundMusic();
    playEndSound();
}

void gameComplete(void) {
    if (game.state == GAME_STATE_COMPLETED) {
        return;
    }
    if (DEBUG_MODE) {
        riv_printf("Game completed at frame %d: score=%d\n", riv->frame, game.score);
    }

    scoringRefreshOutcard(&game);
    game.state = GAME_STATE_COMPLETED;
    riv->quit_frame = riv->frame + 5 * riv->target_fps;
    stopBackgroundMusic();
    playVictoryFanfare();
}
