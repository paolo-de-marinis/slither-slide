#ifndef TECHNICAL_VIEW_H
#define TECHNICAL_VIEW_H

#include "joint_point.h"
#include "snake_geometry.h"

#include <stdbool.h>

void technicalViewDraw(const JointPoint *worldJoints,
                       int jointCount,
                       const SnakeGeometry *geometry,
                       int activeSpan,
                       float t,
                       bool manualSpanSelection);

#endif
