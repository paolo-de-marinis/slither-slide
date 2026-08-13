#include "snake_geometry.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static bool nearlyEqual(float first, float second) {
    return fabsf(first - second) < 0.0001f;
}

int main(void) {
    JointPoint joints[3] = {
        {.x = 0.0f, .y = 0.0f, .angle = 0.0f},
        {.x = 10.0f, .y = 0.0f, .angle = 0.0f},
        {.x = 20.0f, .y = 0.0f, .angle = 0.0f},
    };
    SnakeGeometry geometry;
    assert(snakeGeometryBuild(joints, 3, &geometry));
    assert(geometry.controlPointCount == 7);
    assert(geometry.curveSampleCount == 14);

    assert(nearlyEqual(geometry.controlPoints[0].x, 20.0f));
    assert(nearlyEqual(geometry.controlPoints[0].y, 5.0f));
    assert(nearlyEqual(geometry.controlPoints[2].x, 0.0f));
    assert(nearlyEqual(geometry.controlPoints[2].y, 8.0f));
    assert(nearlyEqual(geometry.controlPoints[3].x, 8.0f));
    assert(nearlyEqual(geometry.controlPoints[3].y, 0.0f));
    assert(nearlyEqual(geometry.controlPoints[4].x, 0.0f));
    assert(nearlyEqual(geometry.controlPoints[4].y, -8.0f));

    for (int span = 0; span < geometry.controlPointCount; span++) {
        SnakeGeometryPoint start;
        SnakeGeometryPoint midpoint;
        SnakeGeometryPoint end;
        SnakeGeometryPoint nextStart;
        assert(snakeGeometryEvaluateSpan(&geometry, span, 0.0f, &start));
        assert(snakeGeometryEvaluateSpan(&geometry, span, 0.5f, &midpoint));
        assert(snakeGeometryEvaluateSpan(&geometry, span, 1.0f, &end));
        assert(snakeGeometryEvaluateSpan(
            &geometry, (span + 1) % geometry.controlPointCount, 0.0f, &nextStart));
        assert(nearlyEqual(end.x, nextStart.x));
        assert(nearlyEqual(end.y, nextStart.y));
        assert(nearlyEqual(start.x, geometry.curveSamples[span * 2].x));
        assert(nearlyEqual(start.y, geometry.curveSamples[span * 2].y));
        assert(nearlyEqual(midpoint.x, geometry.curveSamples[span * 2 + 1].x));
        assert(nearlyEqual(midpoint.y, geometry.curveSamples[span * 2 + 1].y));
    }

    SnakeGeometryPoint midpoint;
    assert(snakeGeometryEvaluateSpan(&geometry, 0, 0.5f, &midpoint));
    float expectedX = (geometry.controlPoints[0].x + 23.0f * geometry.controlPoints[1].x +
                       23.0f * geometry.controlPoints[2].x + geometry.controlPoints[3].x) /
                      48.0f;
    float expectedY = (geometry.controlPoints[0].y + 23.0f * geometry.controlPoints[1].y +
                       23.0f * geometry.controlPoints[2].y + geometry.controlPoints[3].y) /
                      48.0f;
    assert(nearlyEqual(midpoint.x, expectedX));
    assert(nearlyEqual(midpoint.y, expectedY));

    assert(!snakeGeometryBuild(NULL, 3, &geometry));
    assert(!snakeGeometryBuild(joints, 1, &geometry));
    assert(geometry.controlPointCount == 0);
    assert(geometry.curveSampleCount == 0);

    puts("snake geometry: ok");
    return 0;
}
