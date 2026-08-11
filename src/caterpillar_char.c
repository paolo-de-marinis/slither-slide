#include "caterpillar_char.h"
#include <math.h>

static const float BODY_WIDTH = 6.0f;

void drawCaterpillarBody(const JointPoint *joints, int jointCount, float bodyScale) {
    for (int i = jointCount - 1; i >= 0; i--) {
        float screenX = 0.0f;
        float screenY = 0.0f;
        worldToScreen(joints[i].x, joints[i].y, &screenX, &screenY);

        float width = BODY_WIDTH * bodyScale;
        riv_draw_circle_fill((int)screenX, (int)screenY, width * 2.2f, RIV_COLOR_DARKGREEN);
        riv_draw_circle_fill((int)screenX, (int)screenY, width * 1.8f, RIV_COLOR_GREEN);
        riv_draw_circle_fill((int)screenX, (int)screenY, width * 1.4f, RIV_COLOR_LIGHTGREEN);
    }
}

void drawCaterpillarHead(float x, float y, float angle, float radius) {
    float screenX = 0.0f;
    float screenY = 0.0f;
    worldToScreen(x, y, &screenX, &screenY);

    riv_draw_circle_fill((int)screenX, (int)screenY, radius * 2.2f, RIV_COLOR_DARKGREEN);
    riv_draw_circle_fill((int)screenX, (int)screenY, radius * 1.8f, RIV_COLOR_GREEN);
    riv_draw_circle_fill((int)screenX, (int)screenY, radius * 1.4f, RIV_COLOR_LIGHTGREEN);

    float eyeAngle = PI / 4.0f;
    float eyeDistance = radius * 0.6f;

    float leftEyeX = screenX + cosf(angle + eyeAngle) * eyeDistance;
    float leftEyeY = screenY + sinf(angle + eyeAngle) * eyeDistance;
    riv_draw_circle_fill((int)leftEyeX, (int)leftEyeY, 3, RIV_COLOR_WHITE);
    riv_draw_circle_fill((int)leftEyeX, (int)leftEyeY, 2, RIV_COLOR_BLACK);

    float rightEyeX = screenX + cosf(angle - eyeAngle) * eyeDistance;
    float rightEyeY = screenY + sinf(angle - eyeAngle) * eyeDistance;
    riv_draw_circle_fill((int)rightEyeX, (int)rightEyeY, 3, RIV_COLOR_WHITE);
    riv_draw_circle_fill((int)rightEyeX, (int)rightEyeY, 2, RIV_COLOR_BLACK);
}
