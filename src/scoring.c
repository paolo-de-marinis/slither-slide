#include "scoring.h"

#include "levels.h"

#include <stdio.h>
#include <string.h>

#define MAX_LEVEL_INFO_SIZE 64
#define MAX_SCORE_DETAILS_SIZE 512

static void captureActiveTime(GameData *state);
static void updateLevelScore(GameData *state, int level);
static void updateTotalScore(GameData *state);
static int buildLevelDetails(const GameData *state, char *details, size_t detailsSize);

void scoringInitialize(GameData *state) {
    state->score = 0;
    state->levelEntryTick = 0;
    state->levelTimerRunning = false;
    memset(state->levelScores, 0, sizeof(state->levelScores));
    memset(state->levelElapsedTicks, 0, sizeof(state->levelElapsedTicks));
    riv->outcard_len =
        riv_snprintf((char *)riv->outcard,
                     RIV_SIZE_OUTCARD,
                     "JSON{\"score\":0,\"apples\":0,\"length\":%d,\"time\":0.0}",
                     INITIAL_SNAKE_LENGTH);
}

void scoringBeginLevel(GameData *state, int level) {
    if (level < 1 || level > MAX_LEVEL) {
        return;
    }
    state->levelEntryTick = state->ticks;
    state->levelTimerRunning = !state->levelCompleted[level - 1];
}

static void captureActiveTime(GameData *state) {
    if (!state->levelTimerRunning || state->currentLevel < 1 || state->currentLevel > MAX_LEVEL) {
        return;
    }

    int elapsed = state->ticks - state->levelEntryTick;
    if (elapsed > 0) {
        state->levelElapsedTicks[state->currentLevel - 1] += elapsed;
    }
    state->levelEntryTick = state->ticks;
}

static void updateLevelScore(GameData *state, int level) {
    if (level < 1 || level > MAX_LEVEL || state->levelCompleted[level - 1]) {
        return;
    }

    int index = level - 1;
    float levelTime = state->levelElapsedTicks[index] / (float)TARGET_FPS;
    int timePenalty = (int)(levelTime * 2.0f);
    int lengthPenalty = state->jointCount - INITIAL_SNAKE_LENGTH;
    state->levelScores[index] = state->levelItems[index] * MAP_SIZE * 2 - lengthPenalty - timePenalty;
}

static void updateTotalScore(GameData *state) {
    state->score = 0;
    for (int index = 0; index < MAX_LEVEL; index++) {
        state->score += state->levelScores[index];
    }
}

void scoringPauseLevel(GameData *state) {
    captureActiveTime(state);
    updateLevelScore(state, state->currentLevel);
    state->levelTimerRunning = false;
    updateTotalScore(state);
    scoringRefreshOutcard(state);
}

void scoringCompleteLevel(GameData *state, int level, int bonusPoints) {
    if (level < 1 || level > MAX_LEVEL || state->levelCompleted[level - 1]) {
        return;
    }

    captureActiveTime(state);
    updateLevelScore(state, level);
    state->levelScores[level - 1] += bonusPoints;
    state->levelCompleted[level - 1] = true;
    if (state->currentLevel == level) {
        state->levelTimerRunning = false;
    }
    updateTotalScore(state);
    scoringRefreshOutcard(state);
}

void scoringUpdate(GameData *state) {
    captureActiveTime(state);
    updateLevelScore(state, state->currentLevel);
    updateTotalScore(state);
    scoringRefreshOutcard(state);
}

static int buildLevelDetails(const GameData *state, char *details, size_t detailsSize) {
    int totalItems = 0;
    for (int index = 0; index < MAX_LEVEL; index++) {
        int level = index + 1;
        totalItems += state->levelItems[index];
        float levelTime = state->levelElapsedTicks[index] / (float)TARGET_FPS;

        char levelInfo[MAX_LEVEL_INFO_SIZE] = "";
        snprintf(levelInfo,
                 sizeof(levelInfo),
                 "%s\"l%d\":[%d,%d,%d,%.1f]",
                 index > 0 ? "," : "",
                 level,
                 state->levelItems[index],
                 state->levelCompleted[index] ? 1 : 0,
                 state->levelScores[index],
                 levelTime);

        if (strlen(details) + strlen(levelInfo) < detailsSize) {
            strcat(details, levelInfo);
        }
    }
    return totalItems;
}

void scoringRefreshOutcard(GameData *state) {
    char levelDetails[MAX_SCORE_DETAILS_SIZE] = "";
    int totalItems = buildLevelDetails(state, levelDetails, sizeof(levelDetails));
    float totalTime = state->ticks / (float)TARGET_FPS;
    int level = state->currentLevel > 0 ? state->currentLevel : 1;

    riv->outcard_len =
        riv_snprintf((char *)riv->outcard,
                     RIV_SIZE_OUTCARD,
                     "JSON{\"score\":%d,\"a\":%d,\"l\":%d,\"t\":%.1f,\"c\":%d,\"lvl\":{%s}}",
                     state->score,
                     totalItems,
                     state->jointCount,
                     totalTime,
                     level,
                     levelDetails);
}
