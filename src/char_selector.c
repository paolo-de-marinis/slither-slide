#include "char_selector.h"
#include "snake_char.h"
#include "caterpillar_char.h"
#include "joint_point.h"
#include "ui_constants.h"
#include <math.h>
#include <string.h>

static SkinType currentSkin = SKIN_SNAKE;
static bool skinSelected = false;

#define PREVIEW_SCALE 0.8f
#define SNAKE_PREVIEW_JOINTS 30
#define CATERPILLAR_PREVIEW_JOINTS 6

static void drawSkinOptions(void);
static void
buildPreviewJoints(JointPoint *joints, int jointCount, float previewLength, float curveHeight);
static void drawSkinPreview(void);
static void drawSelectionInstructions(void);
static bool startButtonPressed(void);

static riv_waveform_desc navigationSound = {
    .type = RIV_WAVEFORM_PULSE,
    .attack = 0.01f,
    .decay = 0.01f,
    .sustain = 0.05f,
    .release = 0.01f,
    .start_frequency = RIV_NOTE_C5,
    .end_frequency = RIV_NOTE_C5,
    .amplitude = 0.2f,
    .sustain_level = 0.3f,
};

void initializeSkinSelector(void) {
    currentSkin = SKIN_SNAKE;
    skinSelected = false;
}

void drawSkinSelectionMenu(void) {
    riv_draw_text("SLITHER SLIDE",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  TOP_FIFTH - 20,
                  TITLE_SCALE,
                  RIV_COLOR_LIGHTGREEN);
    riv_draw_text("SELECT SKIN",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  TOP_FIFTH + 10,
                  NORMAL_SCALE,
                  RIV_COLOR_GREEN);

    drawSkinOptions();
    drawSkinPreview();
    drawSelectionInstructions();
}

static void drawSkinOptions(void) {
    int snakeWidth = (int)(strlen("SNAKE") * 5 * NORMAL_SCALE);
    int caterpillarWidth = (int)(strlen("CATERPILLAR") * 5 * NORMAL_SCALE);

    const char *marker = ">";
    int markerOffset = (currentSkin == SKIN_SNAKE) ? snakeWidth : caterpillarWidth;
    int markerX = CENTER_X - markerOffset / 2 - 15;
    int markerY = MIDDLE - STANDARD_SPACING + (currentSkin * STANDARD_SPACING);
    riv_draw_text(marker,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_LEFT,
                  markerX,
                  markerY,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTGREEN);

    uint32_t snakeColor = (currentSkin == SKIN_SNAKE) ? RIV_COLOR_LIGHTGREEN : RIV_COLOR_DARKGREEN;
    uint32_t caterpillarColor =
        (currentSkin == SKIN_CATERPILLAR) ? RIV_COLOR_LIGHTGREEN : RIV_COLOR_DARKGREEN;

    riv_draw_text("SNAKE",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  MIDDLE - STANDARD_SPACING,
                  NORMAL_SCALE,
                  snakeColor);
    riv_draw_text("CATERPILLAR",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  MIDDLE,
                  NORMAL_SCALE,
                  caterpillarColor);
}

static void
buildPreviewJoints(JointPoint *joints, int jointCount, float previewLength, float curveHeight) {
    for (int i = 0; i < jointCount; i++) {
        float t = (float)i / (jointCount - 1);
        joints[i].x = CENTER_X + t * previewLength - previewLength / 2.0f;

        if (currentSkin == SKIN_SNAKE) {
            joints[i].y = PREVIEW_Y + sinf(t * PI * 2.0f) * curveHeight;
        } else {
            joints[i].y = PREVIEW_Y + sinf(t * PI) * curveHeight;
        }

        if (i > 0) {
            float deltaX = joints[i].x - joints[i - 1].x;
            float deltaY = joints[i].y - joints[i - 1].y;
            joints[i - 1].angle = atan2f(deltaY, deltaX) + PI;
        }
    }
    joints[jointCount - 1].angle = joints[jointCount - 2].angle;
}

static void drawSkinPreview(void) {
    JointPoint previewJoints[SNAKE_PREVIEW_JOINTS];
    float previewLength = 30.0f;
    int jointCount = CATERPILLAR_PREVIEW_JOINTS;
    if (currentSkin == SKIN_SNAKE) {
        previewLength = 140.0f;
        jointCount = SNAKE_PREVIEW_JOINTS;
    }

    float curveHeight = 8.0f;
    buildPreviewJoints(previewJoints, jointCount, previewLength, curveHeight);

    if (currentSkin == SKIN_SNAKE) {
        drawSnakeBody(previewJoints, jointCount);
        drawSnakeHead(previewJoints[0].x, previewJoints[0].y, previewJoints[0].angle);
        static float tongueAnimation = 0.0f;
        tongueAnimation = (sinf(riv->frame * 0.1f) + 1.0f) * 0.5f;
        drawSnakeTongue(previewJoints[0].x,
                        previewJoints[0].y,
                        previewJoints[0].angle,
                        tongueAnimation * PREVIEW_SCALE);
    } else {
        drawCaterpillarBody(previewJoints, jointCount, PREVIEW_SCALE);
        drawCaterpillarHead(previewJoints[0].x,
                            previewJoints[0].y,
                            previewJoints[0].angle,
                            CATERPILLAR_HEAD_RADIUS * PREVIEW_SCALE);
    }
}

static void drawSelectionInstructions(void) {
    riv_draw_text("UP/DOWN to select",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  PREVIEW_Y + 30,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTRED);
    riv_draw_text("PRESS ANY KEY to start",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  PREVIEW_Y + 50,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTRED);

    riv_draw_text("RIVES Jam #3",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  SCREEN_HEIGHT - 30,
                  NORMAL_SCALE,
                  RIV_COLOR_GREY);
    riv_draw_text("Theme: Slide",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  SCREEN_HEIGHT - 15,
                  NORMAL_SCALE,
                  RIV_COLOR_GREY);
}

void updateSkinSelection(void) {
    if (riv->keys[RIV_GAMEPAD_UP].press) {
        currentSkin = (currentSkin - 1 + SKIN_COUNT) % SKIN_COUNT;
        riv_waveform(&navigationSound);
    }
    if (riv->keys[RIV_GAMEPAD_DOWN].press) {
        currentSkin = (currentSkin + 1) % SKIN_COUNT;
        riv_waveform(&navigationSound);
    }

    if (startButtonPressed()) {
        skinSelected = true;
    }
}

static bool startButtonPressed(void) {
    return riv->keys[RIV_GAMEPAD_A1].press || riv->keys[RIV_GAMEPAD_A2].press ||
           riv->keys[RIV_GAMEPAD_A3].press || riv->keys[RIV_GAMEPAD_A4].press ||
           riv->keys[RIV_GAMEPAD_L1].press || riv->keys[RIV_GAMEPAD_R1].press ||
           riv->keys[RIV_GAMEPAD_L2].press || riv->keys[RIV_GAMEPAD_R2].press ||
           riv->keys[RIV_GAMEPAD_SELECT].press || riv->keys[RIV_GAMEPAD_START].press;
}

SkinType getCurrentSkin(void) {
    return currentSkin;
}

bool isSkinSelected(void) {
    return skinSelected;
}
