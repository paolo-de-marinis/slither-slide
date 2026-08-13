#ifndef CATERPILLAR_CHAR_H
#define CATERPILLAR_CHAR_H

#include "riv.h"
#include "joint_point.h"
#include "camera.h"

// Simplified constants to match snake
#define CATERPILLAR_HEAD_COLLISION_RADIUS 7.0f

// Core drawing functions
void drawCaterpillarBody(const JointPoint *joints, int jointCount, float bodyScale);
void drawCaterpillarHead(float x, float y, float angle, float radius);

#endif // CATERPILLAR_CHAR_H
