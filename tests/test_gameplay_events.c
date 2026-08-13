#include "snake_motion.h"

#include <assert.h>
#include <stdio.h>

GameData game;

static int testLevel;
static int spawnCalls;
static int completeCalls;
static int endCalls;
static bool doorCompleted;
static bool spawnResult;

int getCurrentLevel(void) {
    return testLevel;
}

bool getLevelConfig(int level, LevelConfig *config) {
    if (level < 1 || level > MAX_LEVEL || config == NULL) {
        return false;
    }
    *config = (LevelConfig){.requiredItems = level == MAX_LEVEL ? 50 : 5,
                            .moveDelay = 6,
                            .growthRate = 1,
                            .bonusPoints = 100};
    return true;
}

bool checkDoorState(void) {
    return doorCompleted;
}

bool handleLevelTransition(void) {
    return true;
}

bool collectibleSpawn(const GameData *state) {
    (void)state;
    spawnCalls++;
    return spawnResult;
}

bool collectibleTouchesHead(const GameData *state, float radius) {
    (void)state;
    (void)radius;
    return false;
}

float collisionHeadRadius(void) {
    return 6.0f;
}

bool collisionAtNextHead(const GameData *state, riv_vec2i position) {
    (void)state;
    (void)position;
    return false;
}

void bodyChainGrow(GameData *state) {
    state->jointCount++;
}

void playEatSound(void) {
}

void gameComplete(void) {
    completeCalls++;
    game.state = GAME_STATE_COMPLETED;
}

void gameEnd(void) {
    endCalls++;
    game.state = GAME_STATE_OVER;
}

int main(void) {
    game = (GameData){.state = GAME_STATE_PLAYING, .jointCount = INITIAL_SNAKE_LENGTH};
    testLevel = MAX_LEVEL;
    game.levelItems[MAX_LEVEL - 1] = 49;
    doorCompleted = true;
    spawnResult = true;
    assert(!snakeCollectItem(&game));
    assert(game.levelItems[MAX_LEVEL - 1] == 50);
    assert(game.jointCount == INITIAL_SNAKE_LENGTH + 1);
    assert(completeCalls == 1);
    assert(spawnCalls == 0);
    assert(endCalls == 0);

    game = (GameData){.state = GAME_STATE_PLAYING, .jointCount = INITIAL_SNAKE_LENGTH};
    testLevel = 1;
    doorCompleted = false;
    spawnResult = false;
    assert(!snakeCollectItem(&game));
    assert(spawnCalls == 1);
    assert(endCalls == 1);

    puts("gameplay completion and spawn failure: ok");
    return 0;
}
