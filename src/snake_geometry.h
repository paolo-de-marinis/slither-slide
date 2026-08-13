#ifndef SNAKE_GEOMETRY_H
#define SNAKE_GEOMETRY_H

#include "joint_point.h"

#include <stdbool.h>

enum {
    SNAKE_CONTROL_POINTS_MAX = MAX_JOINTS * 2 + 1,
    SNAKE_SAMPLES_PER_SPAN = 2,
    SNAKE_CURVE_SAMPLES_MAX = SNAKE_CONTROL_POINTS_MAX * SNAKE_SAMPLES_PER_SPAN
};

typedef struct {
    float x;
    float y;
} SnakeGeometryPoint;

typedef struct {
    SnakeGeometryPoint controlPoints[SNAKE_CONTROL_POINTS_MAX];
    int controlPointCount;
    SnakeGeometryPoint curveSamples[SNAKE_CURVE_SAMPLES_MAX];
    int curveSampleCount;
} SnakeGeometry;

float snakeBodyWidth(int segmentIndex);
void snakeOffsetPosition(const JointPoint *joints,
                         int index,
                         float angleOffset,
                         float *x,
                         float *y);
bool snakeGeometryBuild(const JointPoint *joints, int jointCount, SnakeGeometry *geometry);
bool snakeGeometryEvaluateSpan(const SnakeGeometry *geometry,
                               int spanIndex,
                               float t,
                               SnakeGeometryPoint *point);

#endif
