#include "snake_char.h"
#include "spline_math.h"
#include "ui_constants.h"
#include <math.h>
#include <stdlib.h>

#define MAX_OUTLINE_POINTS (MAX_JOINTS * 2 + 1)
#define CURVE_POINTS_PER_OUTLINE_POINT 2

typedef struct {
    int x;
    int y;
} CurvePoint;

// Reused every frame to keep large drawing buffers off the stack.
static CurvePoint points[MAX_OUTLINE_POINTS];
static CurvePoint extendedPoints[MAX_OUTLINE_POINTS + 3];
static CurvePoint curvePoints[MAX_OUTLINE_POINTS * CURVE_POINTS_PER_OUTLINE_POINT];
static JointPoint drawJoints[MAX_JOINTS];
static int pointCount = 0;

static void curveVertex(int x, int y);
static void drawSmoothCurve(uint32_t outlineColor);
float getSnakeBodyWidth(int segmentIndex) {
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

void drawSnakeBody(const JointPoint *joints, int jointCount) {
    pointCount = 0;

    for (int i = 0; i < jointCount; i++) {
        float screenX = 0.0f;
        float screenY = 0.0f;
        worldToScreen(joints[i].x, joints[i].y, &screenX, &screenY);
        drawJoints[i].x = screenX;
        drawJoints[i].y = screenY;
        drawJoints[i].angle = joints[i].angle;
    }

    for (int i = jointCount - 1; i >= 0; i--) {
        float x = 0.0f;
        float y = 0.0f;
        getOffsetPosition(&drawJoints[0], i, PI / 2.0f, &x, &y);
        curveVertex((int)x, (int)y);
    }

    float fx = 0.0f;
    float fy = 0.0f;
    getOffsetPosition(&drawJoints[0], 0, 0, &fx, &fy);
    curveVertex((int)fx, (int)fy);

    for (int i = 0; i < jointCount; i++) {
        float x = 0.0f;
        float y = 0.0f;
        getOffsetPosition(&drawJoints[0], i, -PI / 2.0f, &x, &y);
        curveVertex((int)x, (int)y);
    }

    drawSmoothCurve(RIV_COLOR_GREEN);

    for (int i = 1; i < jointCount - 1; i++) {
        float width = getSnakeBodyWidth(i);
        float angle = drawJoints[i].angle;

        const int rowCount = 2;
        const float rowSpacing = width * 0.4f;
        float columnOffset = (i % 2) * 1.0f;

        for (int row = -rowCount; row <= rowCount; row++) {
            float perpendicular = angle + PI / 2.0f;
            float rowY = row * rowSpacing;

            float scaleX = drawJoints[i].x + cosf(perpendicular) * rowY;
            float scaleY = drawJoints[i].y + sinf(perpendicular) * rowY;

            scaleX += cosf(angle) * columnOffset;
            scaleY += sinf(angle) * columnOffset;

            uint32_t scaleColor;
            if (abs(row) == rowCount) {
                scaleColor = RIV_COLOR_LIGHTGREEN;
            } else if (row == 0) {
                scaleColor = RIV_COLOR_GREEN;
            } else {
                scaleColor = RIV_COLOR_WHITE;
            }

            riv_draw_point((int)scaleX, (int)scaleY, scaleColor);
        }
    }
}

void drawSnakeHead(float x, float y, float angle) {
    float screenX = 0.0f;
    float screenY = 0.0f;
    worldToScreen(x, y, &screenX, &screenY);

    float headRadius = getSnakeBodyWidth(0) * 0.9f;
    float headOffset = headRadius * 0.5f;
    float headX = screenX + cosf(angle) * headOffset;
    float headY = screenY + sinf(angle) * headOffset;

    riv_draw_circle_fill((int)headX, (int)headY, (int)headRadius, RIV_COLOR_LIGHTGREEN);

    float noseX = headX + cosf(angle) * SNAKE_NOSE_LENGTH;
    float noseY = headY + sinf(angle) * SNAKE_NOSE_LENGTH;
    riv_draw_line((int)headX, (int)headY, (int)noseX, (int)noseY, RIV_COLOR_LIGHTGREEN);
    riv_draw_circle_fill((int)noseX, (int)noseY, 2, RIV_COLOR_DARKGREEN);

    float eyeAngle = PI / 4.0f;
    float eyeDistance = headRadius * 0.6f;

    float leftEyeX = screenX + cosf(angle + eyeAngle) * eyeDistance;
    float leftEyeY = screenY + sinf(angle + eyeAngle) * eyeDistance;
    float rightEyeX = screenX + cosf(angle - eyeAngle) * eyeDistance;
    float rightEyeY = screenY + sinf(angle - eyeAngle) * eyeDistance;

    riv_draw_circle_fill((int)leftEyeX, (int)leftEyeY, SNAKE_EYE_SIZE, RIV_COLOR_WHITE);
    riv_draw_circle_fill((int)rightEyeX, (int)rightEyeY, SNAKE_EYE_SIZE, RIV_COLOR_WHITE);

    riv_draw_circle_fill((int)leftEyeX, (int)leftEyeY, 1, RIV_COLOR_BLACK);
    riv_draw_circle_fill((int)rightEyeX, (int)rightEyeY, 1, RIV_COLOR_BLACK);
}

void drawSnakeTongue(float x, float y, float angle, float extension) {
    if (extension <= 0.0f) {
        return;
    }

    float screenX = 0.0f;
    float screenY = 0.0f;
    worldToScreen(x, y, &screenX, &screenY);

    float headRadius = getSnakeBodyWidth(0) * 0.9f;
    float headOffset = headRadius * 0.5f;

    float tongueStartX = screenX + cosf(angle) * headOffset + cosf(angle) * SNAKE_NOSE_LENGTH;
    float tongueStartY = screenY + sinf(angle) * headOffset + sinf(angle) * SNAKE_NOSE_LENGTH;

    float baseTongueStem = SNAKE_TONGUE_LENGTH * 0.6f;
    float baseTongueFork = SNAKE_TONGUE_LENGTH * 0.5f;

    float tongueStemLength = baseTongueStem * extension;
    float tongueForkLength = baseTongueFork * extension;
    float tongueFork = PI / 4.0f;

    float forkStartX = tongueStartX + cosf(angle) * tongueStemLength;
    float forkStartY = tongueStartY + sinf(angle) * tongueStemLength;

    riv_draw_line(
        (int)tongueStartX, (int)tongueStartY, (int)forkStartX, (int)forkStartY, RIV_COLOR_RED);

    float leftForkX = forkStartX + cosf(angle + tongueFork) * tongueForkLength;
    float leftForkY = forkStartY + sinf(angle + tongueFork) * tongueForkLength;
    float rightForkX = forkStartX + cosf(angle - tongueFork) * tongueForkLength;
    float rightForkY = forkStartY + sinf(angle - tongueFork) * tongueForkLength;

    riv_draw_line((int)forkStartX, (int)forkStartY, (int)leftForkX, (int)leftForkY, RIV_COLOR_RED);
    riv_draw_line(
        (int)forkStartX, (int)forkStartY, (int)rightForkX, (int)rightForkY, RIV_COLOR_RED);
}

void getOffsetPosition(const JointPoint *joints, int index, float angleOffset, float *x, float *y) {
    float width = getSnakeBodyWidth(index);
    *x = joints[index].x + cosf(joints[index].angle + angleOffset) * width;
    *y = joints[index].y + sinf(joints[index].angle + angleOffset) * width;
}

static void curveVertex(int x, int y) {
    if (pointCount < MAX_OUTLINE_POINTS) {
        points[pointCount].x = x;
        points[pointCount].y = y;
        pointCount++;
    }
}

static void drawSmoothCurve(uint32_t outlineColor) {
    if (pointCount < 4) {
        return;
    }

    for (int i = 0; i < pointCount; i++) {
        extendedPoints[i] = points[i];
    }
    extendedPoints[pointCount] = points[0];
    extendedPoints[pointCount + 1] = points[1];
    extendedPoints[pointCount + 2] = points[2];

    int curvePointCount = 0;
    float step = 0.5f;

    for (int i = 0; i < pointCount; i++) {
        for (float t = 0; t < 1.0f; t += step) {
            float weights[4];
            cubicUniformBSplineBasis(t, weights);
            float x = weights[0] * extendedPoints[i].x +
                      weights[1] * extendedPoints[i + 1].x +
                      weights[2] * extendedPoints[i + 2].x +
                      weights[3] * extendedPoints[i + 3].x;
            float y = weights[0] * extendedPoints[i].y +
                      weights[1] * extendedPoints[i + 1].y +
                      weights[2] * extendedPoints[i + 2].y +
                      weights[3] * extendedPoints[i + 3].y;

            curvePoints[curvePointCount].x = (int)x;
            curvePoints[curvePointCount].y = (int)y;
            curvePointCount++;
        }
    }

    int halfCurvePoints = curvePointCount / 2;
    for (int i = 0; i < halfCurvePoints - 1; i++) {
        int leftIndex = i;
        int rightIndex = curvePointCount - 1 - i;
        int nextLeftIndex = leftIndex + 1;
        int nextRightIndex = curvePointCount - 2 - i;

        riv_draw_triangle_fill(curvePoints[leftIndex].x,
                               curvePoints[leftIndex].y,
                               curvePoints[nextLeftIndex].x,
                               curvePoints[nextLeftIndex].y,
                               curvePoints[rightIndex].x,
                               curvePoints[rightIndex].y,
                               RIV_COLOR_LIGHTGREEN);

        riv_draw_triangle_fill(curvePoints[nextLeftIndex].x,
                               curvePoints[nextLeftIndex].y,
                               curvePoints[nextRightIndex].x,
                               curvePoints[nextRightIndex].y,
                               curvePoints[rightIndex].x,
                               curvePoints[rightIndex].y,
                               RIV_COLOR_LIGHTGREEN);
    }

    for (int i = 0; i < curvePointCount - 1; i++) {
        riv_draw_line(curvePoints[i].x,
                      curvePoints[i].y,
                      curvePoints[i + 1].x,
                      curvePoints[i + 1].y,
                      outlineColor);
    }
    riv_draw_line(curvePoints[curvePointCount - 1].x,
                  curvePoints[curvePointCount - 1].y,
                  curvePoints[0].x,
                  curvePoints[0].y,
                  outlineColor);
}

void updateSnakeAnimation(SnakeAnimationState *state) {
    state->tongueTimer++;
    if (state->tongueTimer >= 4) {
        state->tongueTimer = 0;

        if (state->tongueExtending) {
            state->tongueExtension += 0.25f;
            if (state->tongueExtension >= 1.0f) {
                state->tongueExtension = 1.0f;
                state->tongueExtending = false;
            }
        } else {
            state->tongueExtension -= 0.25f;
            if (state->tongueExtension <= 0.0f) {
                state->tongueExtension = 0.0f;
                state->tongueExtending = true;
            }
        }
    }
}

void initializeSnakeAnimation(SnakeAnimationState *state) {
    state->tongueExtension = 0.0f;
    state->tongueExtending = true;
    state->tongueTimer = 0;
}
