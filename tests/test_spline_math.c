#include "spline_math.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static bool nearlyEqual(float first, float second) {
    return fabsf(first - second) < 0.00001f;
}

int main(void) {
    for (int sample = 0; sample <= 100; sample++) {
        float t = sample / 100.0f;
        float weights[4];
        cubicUniformBSplineBasis(t, weights);

        float sum = 0.0f;
        for (int index = 0; index < 4; index++) {
            assert(weights[index] >= 0.0f);
            sum += weights[index];
        }
        assert(nearlyEqual(sum, 1.0f));
    }

    float start[4];
    float midpoint[4];
    float end[4];
    cubicUniformBSplineBasis(0.0f, start);
    cubicUniformBSplineBasis(0.5f, midpoint);
    cubicUniformBSplineBasis(1.0f, end);
    assert(nearlyEqual(start[0], 1.0f / 6.0f));
    assert(nearlyEqual(start[1], 4.0f / 6.0f));
    assert(nearlyEqual(start[2], 1.0f / 6.0f));
    assert(nearlyEqual(start[3], 0.0f));
    assert(nearlyEqual(midpoint[0], 1.0f / 48.0f));
    assert(nearlyEqual(midpoint[1], 23.0f / 48.0f));
    assert(nearlyEqual(midpoint[2], 23.0f / 48.0f));
    assert(nearlyEqual(midpoint[3], 1.0f / 48.0f));
    assert(nearlyEqual(end[0], 0.0f));
    assert(nearlyEqual(end[1], 1.0f / 6.0f));
    assert(nearlyEqual(end[2], 4.0f / 6.0f));
    assert(nearlyEqual(end[3], 1.0f / 6.0f));

    puts("B-spline basis invariants: ok");
    return 0;
}
