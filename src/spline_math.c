#include "spline_math.h"

void cubicUniformBSplineBasis(float t, float weights[4]) {
    float oneMinusT = 1.0f - t;
    weights[0] = oneMinusT * oneMinusT * oneMinusT / 6.0f;
    weights[1] = (3.0f * t * t * t - 6.0f * t * t + 4.0f) / 6.0f;
    weights[2] = (-3.0f * t * t * t + 3.0f * t * t + 3.0f * t + 1.0f) / 6.0f;
    weights[3] = t * t * t / 6.0f;
}
