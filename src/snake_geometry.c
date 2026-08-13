#include "snake_geometry.h"

#include "spline_math.h"

#include <math.h>
#include <stddef.h>

static void appendControlPoint(SnakeGeometry *geometry, float x, float y) {
    if (geometry->controlPointCount >= SNAKE_CONTROL_POINTS_MAX) {
        return;
    }
    geometry->controlPoints[geometry->controlPointCount].x = (float)(int)x;
    geometry->controlPoints[geometry->controlPointCount].y = (float)(int)y;
    geometry->controlPointCount++;
}

float snakeBodyWidth(int segmentIndex) {
    if (segmentIndex == 0) {
        return 8.0f;
    }
    if (segmentIndex == 1) {
        return 7.0f;
    }

    float baseWidth = 6.0f;
    float taper = segmentIndex / 24.0f;
    return fmaxf(baseWidth - taper, 4.0f);
}

void snakeOffsetPosition(const JointPoint *joints,
                         int index,
                         float angleOffset,
                         float *x,
                         float *y) {
    float width = snakeBodyWidth(index);
    *x = joints[index].x + cosf(joints[index].angle + angleOffset) * width;
    *y = joints[index].y + sinf(joints[index].angle + angleOffset) * width;
}

bool snakeGeometryEvaluateSpan(const SnakeGeometry *geometry,
                               int spanIndex,
                               float t,
                               SnakeGeometryPoint *point) {
    if (geometry == NULL || point == NULL || geometry->controlPointCount < 4 ||
        spanIndex < 0 || spanIndex >= geometry->controlPointCount || t < 0.0f || t > 1.0f) {
        return false;
    }

    float weights[4];
    cubicUniformBSplineBasis(t, weights);
    point->x = 0.0f;
    point->y = 0.0f;
    for (int offset = 0; offset < 4; offset++) {
        int controlIndex = (spanIndex + offset) % geometry->controlPointCount;
        point->x += weights[offset] * geometry->controlPoints[controlIndex].x;
        point->y += weights[offset] * geometry->controlPoints[controlIndex].y;
    }
    return true;
}

bool snakeGeometryBuild(const JointPoint *joints, int jointCount, SnakeGeometry *geometry) {
    if (geometry == NULL) {
        return false;
    }
    geometry->controlPointCount = 0;
    geometry->curveSampleCount = 0;
    if (joints == NULL || jointCount < 2 || jointCount > MAX_JOINTS) {
        return false;
    }

    for (int index = jointCount - 1; index >= 0; index--) {
        float x = 0.0f;
        float y = 0.0f;
        snakeOffsetPosition(joints, index, PI / 2.0f, &x, &y);
        appendControlPoint(geometry, x, y);
    }

    float forwardX = 0.0f;
    float forwardY = 0.0f;
    snakeOffsetPosition(joints, 0, 0.0f, &forwardX, &forwardY);
    appendControlPoint(geometry, forwardX, forwardY);

    for (int index = 0; index < jointCount; index++) {
        float x = 0.0f;
        float y = 0.0f;
        snakeOffsetPosition(joints, index, -PI / 2.0f, &x, &y);
        appendControlPoint(geometry, x, y);
    }

    for (int span = 0; span < geometry->controlPointCount; span++) {
        for (int sample = 0; sample < SNAKE_SAMPLES_PER_SPAN; sample++) {
            float t = sample / (float)SNAKE_SAMPLES_PER_SPAN;
            SnakeGeometryPoint point;
            if (!snakeGeometryEvaluateSpan(geometry, span, t, &point)) {
                geometry->controlPointCount = 0;
                geometry->curveSampleCount = 0;
                return false;
            }
            geometry->curveSamples[geometry->curveSampleCount] = point;
            geometry->curveSampleCount++;
        }
    }
    return true;
}
