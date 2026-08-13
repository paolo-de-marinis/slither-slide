#include "snake_char.h"
#include "camera.h"
#include "ui_constants.h"
#include <math.h>
#include <stdlib.h>

// Reused every frame to keep large drawing buffers off the stack.
static SnakeGeometry geometry;
static JointPoint drawJoints[MAX_JOINTS];

static void drawSmoothCurve(uint32_t outlineColor);

void drawSnakeBody(const JointPoint *joints, int jointCount) {
    if (joints == NULL || jointCount < 2 || jointCount > MAX_JOINTS) {
        geometry.controlPointCount = 0;
        geometry.curveSampleCount = 0;
        return;
    }

    for (int i = 0; i < jointCount; i++) {
        float screenX = 0.0f;
        float screenY = 0.0f;
        worldToScreen(joints[i].x, joints[i].y, &screenX, &screenY);
        drawJoints[i].x = screenX;
        drawJoints[i].y = screenY;
        drawJoints[i].angle = joints[i].angle;
    }

    if (!snakeGeometryBuild(drawJoints, jointCount, &geometry)) {
        return;
    }

    drawSmoothCurve(RIV_COLOR_GREEN);

    for (int i = 1; i < jointCount - 1; i++) {
        float width = snakeBodyWidth(i);
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

    float headRadius = snakeBodyWidth(0) * 0.9f;
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

    float headRadius = snakeBodyWidth(0) * 0.9f;
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

const SnakeGeometry *snakeBodyGeometry(void) {
    return &geometry;
}

static void drawSmoothCurve(uint32_t outlineColor) {
    if (geometry.curveSampleCount < 2) {
        return;
    }

    int halfCurvePoints = geometry.curveSampleCount / 2;
    for (int i = 0; i < halfCurvePoints - 1; i++) {
        int leftIndex = i;
        int rightIndex = geometry.curveSampleCount - 1 - i;
        int nextLeftIndex = leftIndex + 1;
        int nextRightIndex = geometry.curveSampleCount - 2 - i;

        riv_draw_triangle_fill((int)geometry.curveSamples[leftIndex].x,
                               (int)geometry.curveSamples[leftIndex].y,
                               (int)geometry.curveSamples[nextLeftIndex].x,
                               (int)geometry.curveSamples[nextLeftIndex].y,
                               (int)geometry.curveSamples[rightIndex].x,
                               (int)geometry.curveSamples[rightIndex].y,
                               RIV_COLOR_LIGHTGREEN);

        riv_draw_triangle_fill((int)geometry.curveSamples[nextLeftIndex].x,
                               (int)geometry.curveSamples[nextLeftIndex].y,
                               (int)geometry.curveSamples[nextRightIndex].x,
                               (int)geometry.curveSamples[nextRightIndex].y,
                               (int)geometry.curveSamples[rightIndex].x,
                               (int)geometry.curveSamples[rightIndex].y,
                               RIV_COLOR_LIGHTGREEN);
    }

    for (int i = 0; i < geometry.curveSampleCount - 1; i++) {
        riv_draw_line((int)geometry.curveSamples[i].x,
                      (int)geometry.curveSamples[i].y,
                      (int)geometry.curveSamples[i + 1].x,
                      (int)geometry.curveSamples[i + 1].y,
                      outlineColor);
    }
    riv_draw_line((int)geometry.curveSamples[geometry.curveSampleCount - 1].x,
                  (int)geometry.curveSamples[geometry.curveSampleCount - 1].y,
                  (int)geometry.curveSamples[0].x,
                  (int)geometry.curveSamples[0].y,
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
