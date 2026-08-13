#include "snake_motion.h"

#include "audio.h"
#include "body_chain.h"
#include "collectible.h"
#include "collision.h"
#include "game.h"
#include "levels.h"

static void readDirectionInput(GameData *state);
static bool moveTail(GameData *state);

static void readDirectionInput(GameData *state) {
    if (riv->keys[RIV_GAMEPAD_UP].press && state->headDirection.y != 1) {
        state->headDirection = (riv_vec2i){0, -1};
    } else if (riv->keys[RIV_GAMEPAD_DOWN].press && state->headDirection.y != -1) {
        state->headDirection = (riv_vec2i){0, 1};
    } else if (riv->keys[RIV_GAMEPAD_LEFT].press && state->headDirection.x != 1) {
        state->headDirection = (riv_vec2i){-1, 0};
    } else if (riv->keys[RIV_GAMEPAD_RIGHT].press && state->headDirection.x != -1) {
        state->headDirection = (riv_vec2i){1, 0};
    }
}

static bool moveTail(GameData *state) {
    if (state->tailPosition.x < 0 || state->tailPosition.x >= WORLD_TILES_X ||
        state->tailPosition.y < 0 || state->tailPosition.y >= WORLD_TILES_Y) {
        gameEnd();
        return false;
    }

    riv_vec2i direction = state->bodyDirections[state->tailPosition.y][state->tailPosition.x];
    state->bodyDirections[state->tailPosition.y][state->tailPosition.x] = (riv_vec2i){0, 0};
    state->tailPosition.x += direction.x;
    state->tailPosition.y += direction.y;
    return true;
}

bool snakeCollectItem(GameData *state) {
    int level = getCurrentLevel();
    LevelConfig config;
    if (!getLevelConfig(level, &config)) {
        gameEnd();
        return false;
    }

    if (state->levelItems[level - 1] < config.requiredItems) {
        state->levelItems[level - 1]++;
    }
    bodyChainGrow(state);
    playEatSound();

    if (checkDoorState()) {
        if (level == MAX_LEVEL) {
            gameComplete();
            return false;
        }
        return true;
    }

    if (!collectibleSpawn(state)) {
        gameEnd();
        return false;
    }
    return true;
}

bool snakeMotionUpdate(GameData *state) {
    readDirectionInput(state);
    state->moveTimer++;
    if (state->moveTimer < state->moveDelay) {
        return true;
    }
    state->moveTimer = 0;

    riv_vec2i nextHeadPosition = {state->headPosition.x + state->headDirection.x,
                                  state->headPosition.y + state->headDirection.y};
    if (collisionAtNextHead(state, nextHeadPosition)) {
        gameEnd();
        return false;
    }

    state->bodyDirections[state->headPosition.y][state->headPosition.x] =
        state->headDirection;
    state->headPosition = nextHeadPosition;
    if (!handleLevelTransition()) {
        gameEnd();
        return false;
    }

    if (collectibleTouchesHead(state, collisionHeadRadius())) {
        if (!snakeCollectItem(state)) {
            return false;
        }
    } else if (!moveTail(state)) {
        return false;
    }

    state->collectibleRotation += 0.1f;
    if (state->collectibleRotation > 2.0f * PI) {
        state->collectibleRotation -= 2.0f * PI;
    }
    return true;
}
