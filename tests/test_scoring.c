#include "scoring.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static riv_context testContext;
riv_context *riv = &testContext;

uint64_t riv_snprintf(char *buffer, uint64_t size, const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int result = vsnprintf(buffer, (size_t)size, format, arguments);
    va_end(arguments);
    return result < 0 ? 0U : (uint64_t)result;
}

int main(void) {
    GameData state = {0};
    state.jointCount = INITIAL_SNAKE_LENGTH;
    state.currentLevel = 1;
    scoringInitialize(&state);
    assert(strcmp((char *)riv->outcard,
                  "JSON{\"score\":0,\"apples\":0,\"length\":20,\"time\":0.0}") == 0);
    assert(riv->outcard_len == strlen((char *)riv->outcard));

    scoringBeginLevel(&state, 1);
    state.levelItems[0] = 5;
    state.ticks = TARGET_FPS;
    scoringCompleteLevel(&state, 1, 100);
    assert(state.levelElapsedTicks[0] == TARGET_FPS);
    assert(state.levelScores[0] == 518);
    assert(state.score == 518);

    state.ticks = 10 * TARGET_FPS;
    state.currentLevel = 2;
    scoringBeginLevel(&state, 2);
    state.levelItems[1] = 1;
    state.jointCount = INITIAL_SNAKE_LENGTH + 1;
    state.ticks += TARGET_FPS;
    scoringUpdate(&state);
    assert(state.levelElapsedTicks[1] == TARGET_FPS);
    assert(state.levelScores[1] == 81);
    assert(state.levelScores[0] == 518);
    assert(state.score == 599);

    scoringPauseLevel(&state);
    state.ticks += 5 * TARGET_FPS;
    scoringBeginLevel(&state, 2);
    state.ticks += TARGET_FPS;
    scoringUpdate(&state);
    assert(state.levelElapsedTicks[1] == 2 * TARGET_FPS);
    assert(state.levelScores[0] == 518);

    puts("per-level scoring: ok");
    return 0;
}
