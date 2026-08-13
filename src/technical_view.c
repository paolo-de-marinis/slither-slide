#include "technical_view.h"

#include "camera.h"
#include "riv.h"
#include "spline_math.h"
#include "ui_constants.h"

#include <stdio.h>

enum {
    PANEL_X = 3,
    PANEL_Y = 193,
    PANEL_WIDTH = 216,
    PANEL_HEIGHT = 60
};

static void drawJointChain(const JointPoint *worldJoints, int jointCount) {
    float previousX = 0.0f;
    float previousY = 0.0f;
    for (int index = 0; index < jointCount; index++) {
        float screenX = 0.0f;
        float screenY = 0.0f;
        worldToScreen(worldJoints[index].x, worldJoints[index].y, &screenX, &screenY);
        if (index > 0) {
            riv_draw_line((int)previousX,
                          (int)previousY,
                          (int)screenX,
                          (int)screenY,
                          RIV_COLOR_PINK);
        }
        riv_draw_circle_fill((int)screenX, (int)screenY, 2, RIV_COLOR_PINK);
        previousX = screenX;
        previousY = screenY;
    }
}

static void drawControlPolygon(const SnakeGeometry *geometry) {
    for (int index = 0; index < geometry->controlPointCount; index++) {
        int nextIndex = (index + 1) % geometry->controlPointCount;
        const SnakeGeometryPoint *point = &geometry->controlPoints[index];
        const SnakeGeometryPoint *next = &geometry->controlPoints[nextIndex];
        riv_draw_line(
            (int)point->x, (int)point->y, (int)next->x, (int)next->y, RIV_COLOR_BLUE);
        riv_draw_circle_fill((int)point->x, (int)point->y, 1, RIV_COLOR_LIGHTBLUE);
    }
}

static void drawCurveSamples(const SnakeGeometry *geometry) {
    for (int index = 0; index < geometry->curveSampleCount; index++) {
        int nextIndex = (index + 1) % geometry->curveSampleCount;
        const SnakeGeometryPoint *point = &geometry->curveSamples[index];
        const SnakeGeometryPoint *next = &geometry->curveSamples[nextIndex];
        riv_draw_line(
            (int)point->x, (int)point->y, (int)next->x, (int)next->y, RIV_COLOR_YELLOW);
        riv_draw_circle_fill((int)point->x, (int)point->y, 1, RIV_COLOR_YELLOW);
    }
}

static void drawActiveSpan(const SnakeGeometry *geometry,
                           int span,
                           float t,
                           SnakeGeometryPoint curvePoint,
                           bool manualSpanSelection) {
    for (int offset = 0; offset < 4; offset++) {
        int controlIndex = (span + offset) % geometry->controlPointCount;
        const SnakeGeometryPoint *point = &geometry->controlPoints[controlIndex];
        riv_draw_circle_line((int)point->x, (int)point->y, 4, RIV_COLOR_ORANGE);
        riv_draw_circle_fill((int)point->x, (int)point->y, 1, RIV_COLOR_LIGHTBLUE);
    }

    riv_draw_circle_fill((int)curvePoint.x, (int)curvePoint.y, 3, RIV_COLOR_WHITE);
    riv_draw_text("C",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  (int)curvePoint.x + 4,
                  (int)curvePoint.y - 4,
                  1,
                  RIV_COLOR_WHITE);

    uint32_t panelColor = (RIV_COLOR_BLACK & 0x00FFFFFF) | 0xD0000000;
    riv_draw_rect_fill(PANEL_X, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT, panelColor);

    riv_draw_text("TECHNICAL VIEW  D/R1",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 4,
                  PANEL_Y + 2,
                  1,
                  RIV_COLOR_WHITE);

    char line[48];
    snprintf(line,
             sizeof(line),
             "SPAN i=%d/%d  t=%.2f  %s",
             span,
             geometry->controlPointCount - 1,
             t,
             manualSpanSelection ? "MANUAL" : "AUTO");
    riv_draw_text(line,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 4,
                  PANEL_Y + 10,
                  1,
                  RIV_COLOR_WHITE);

    riv_draw_text("JOINTS",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 4,
                  PANEL_Y + 18,
                  1,
                  RIV_COLOR_PINK);
    riv_draw_text("ALL CONTROLS",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 42,
                  PANEL_Y + 18,
                  1,
                  RIV_COLOR_LIGHTBLUE);
    riv_draw_text("SPLINE/SAMPLES",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 124,
                  PANEL_Y + 18,
                  1,
                  RIV_COLOR_YELLOW);

    riv_draw_text("ACTIVE SPAN P_i..P_i+3",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 4,
                  PANEL_Y + 26,
                  1,
                  RIV_COLOR_ORANGE);
    riv_draw_text("C_i(t)",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 154,
                  PANEL_Y + 26,
                  1,
                  RIV_COLOR_WHITE);

    riv_draw_text("L2/R2: PREV/NEXT SPAN",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 4,
                  PANEL_Y + 34,
                  1,
                  RIV_COLOR_WHITE);

    float weights[4];
    cubicUniformBSplineBasis(t, weights);
    snprintf(line, sizeof(line), "B0 %.3f   B1 %.3f", weights[0], weights[1]);
    riv_draw_text(line,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 4,
                  PANEL_Y + 42,
                  1,
                  RIV_COLOR_WHITE);
    snprintf(line, sizeof(line), "B2 %.3f   B3 %.3f", weights[2], weights[3]);
    riv_draw_text(line,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  PANEL_X + 4,
                  PANEL_Y + 50,
                  1,
                  RIV_COLOR_WHITE);
}

void technicalViewDraw(const JointPoint *worldJoints,
                       int jointCount,
                       const SnakeGeometry *geometry,
                       int activeSpan,
                       float t,
                       bool manualSpanSelection) {
    if (worldJoints == NULL || jointCount < 2 || geometry == NULL ||
        geometry->controlPointCount < 4 || geometry->curveSampleCount < 2) {
        return;
    }

    drawJointChain(worldJoints, jointCount);
    drawControlPolygon(geometry);
    drawCurveSamples(geometry);

    int span = activeSpan % geometry->controlPointCount;
    if (span < 0) {
        span += geometry->controlPointCount;
    }
    SnakeGeometryPoint curvePoint;
    if (snakeGeometryEvaluateSpan(geometry, span, t, &curvePoint)) {
        drawActiveSpan(geometry, span, t, curvePoint, manualSpanSelection);
    }
}
