#include "body_chain.h"
#include "walls.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

float getSnakeBodyWidth(int segmentIndex) {
    (void)segmentIndex;
    return 4.0f;
}

int wallsGetCount(void) {
    return 0;
}

const Wall *wallsGet(int index) {
    (void)index;
    return NULL;
}

bool wallCircleContact(const Wall *wall,
                       float centerX,
                       float centerY,
                       float radius,
                       WallContact *contact) {
    (void)wall;
    (void)centerX;
    (void)centerY;
    (void)radius;
    (void)contact;
    return false;
}

static bool nearlyEqual(float first, float second) {
    return fabsf(first - second) < 0.0001f;
}

int main(void) {
    GameData state = {0};
    state.headPosition = (riv_vec2i){10, 12};
    state.headDirection = (riv_vec2i){1, 0};
    state.jointCount = 6;
    for (int index = 0; index < state.jointCount; index++) {
        state.joints[index] =
            (JointPoint){90.0f - index * 5.0f, 80.0f + index * 2.0f, 123.0f};
    }

    bodyChainUpdate(&state);
    assert(nearlyEqual(state.joints[0].x, 63.0f));
    assert(nearlyEqual(state.joints[0].y, 75.0f));

    for (int index = 1; index < state.jointCount; index++) {
        float expected = atan2f(state.joints[index - 1].y - state.joints[index].y,
                                state.joints[index - 1].x - state.joints[index].x);
        assert(nearlyEqual(state.joints[index].angle, expected));
    }
    float expectedHead = atan2f(state.joints[0].y - state.joints[1].y,
                                state.joints[0].x - state.joints[1].x);
    assert(nearlyEqual(state.joints[0].angle, expectedHead));

    puts("body-chain invariants: ok");
    return 0;
}
