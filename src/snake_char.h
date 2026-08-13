#ifndef SNAKE_CHAR_H
#define SNAKE_CHAR_H

#include "riv.h"
#include "joint_point.h"
#include "camera.h"

// Constants specific to snake rendering
#define SNAKE_HEAD_COLLISION_RADIUS 6.0f
#define SNAKE_TONGUE_LENGTH 8
#define SNAKE_NOSE_LENGTH 4
#define SNAKE_EYE_SIZE 2

// Core drawing functions
void drawSnakeBody(const JointPoint *joints, int jointCount);
void drawSnakeHead(float x, float y, float angle);
void drawSnakeTongue(float x, float y, float angle, float extension);

// Helper functions
float getSnakeBodyWidth(int segmentIndex);
void getOffsetPosition(const JointPoint *joints, int index, float angleOffset,
                       float *x, float *y);

// Animation parameters
typedef struct {
    float tongueExtension;
    bool tongueExtending;
    int tongueTimer;
} SnakeAnimationState;

void updateSnakeAnimation(SnakeAnimationState *state);
void initializeSnakeAnimation(SnakeAnimationState *state);

#endif // SNAKE_CHAR_H
