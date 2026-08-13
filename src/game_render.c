#include "game_render.h"

#include "caterpillar_char.h"
#include "char_selector.h"
#include "collectible.h"
#include "collision.h"
#include "game_state.h"
#include "levels.h"
#include "snake_char.h"
#include "ui_constants.h"
#include "walls.h"

#include <math.h>
#include <stdio.h>

static void drawHud(void);
static void drawCharacter(void);
static void drawDebugGeometry(void);
static void drawGameOver(void);
static void drawGameCompleted(void);
static void drawDitheredOverlay(uint32_t color);

void gameDraw(void) {
    riv_clear(RIV_COLOR_DARKSLATE);
    wallsDraw();

    if (game.state == GAME_STATE_CHAR_SELECT) {
        drawSkinSelectionMenu();
        return;
    }

    drawHud();
    collectibleDraw(&game, game.collectibleRotation);
    drawCharacter();
    if (DEBUG_MODE) {
        drawDebugGeometry();
    }

    if (game.state == GAME_STATE_COMPLETED) {
        drawGameCompleted();
    } else if (game.state == GAME_STATE_OVER) {
        drawGameOver();
    }
}

static void drawHud(void) {
    char scoreText[32] = "";
    snprintf(scoreText, sizeof(scoreText), "SCORE: %d", game.score);
    riv_draw_text(scoreText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  SCREEN_WIDTH - WALL_THICKNESS - 80,
                  WALL_THICKNESS + 5,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTGREEN);

    char levelText[32] = "";
    snprintf(levelText, sizeof(levelText), "LEVEL %d", getCurrentLevel());
    riv_draw_text(levelText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_TOPLEFT,
                  WALL_THICKNESS + 5,
                  WALL_THICKNESS + 5,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTGREEN);
}

static void drawCharacter(void) {
    if (getCurrentSkin() == SKIN_SNAKE) {
        drawSnakeBody(game.joints, game.jointCount);
        drawSnakeHead(game.joints[0].x, game.joints[0].y, game.joints[0].angle);
        drawSnakeTongue(game.joints[0].x,
                        game.joints[0].y,
                        game.joints[0].angle,
                        game.snakeAnimation.tongueExtension);
    } else {
        drawCaterpillarBody(game.joints, game.jointCount, 1.0f);
        drawCaterpillarHead(game.joints[0].x,
                            game.joints[0].y,
                            game.joints[0].angle,
                            CATERPILLAR_HEAD_COLLISION_RADIUS);
    }
}

static void drawDebugGeometry(void) {
    for (int index = 2; index < game.jointCount - 1; index++) {
        uint32_t transparentPink = (RIV_COLOR_PINK & 0x00FFFFFF) | 0x40000000;
        riv_draw_circle_fill(game.joints[index].x,
                             game.joints[index].y,
                             2,
                             RIV_COLOR_PINK);
        riv_draw_circle_fill(game.joints[index].x,
                             game.joints[index].y,
                             TILE_SIZE / 2,
                             transparentPink);
    }

    for (int index = 0; index < game.jointCount; index++) {
        float width = getSnakeBodyWidth(index);
        float angle = game.joints[index].angle;
        float rightX = game.joints[index].x + cosf(angle + PI / 2.0f) * width;
        float rightY = game.joints[index].y + sinf(angle + PI / 2.0f) * width;
        float leftX = game.joints[index].x + cosf(angle - PI / 2.0f) * width;
        float leftY = game.joints[index].y + sinf(angle - PI / 2.0f) * width;
        riv_draw_circle_fill((int)rightX, (int)rightY, 2, RIV_COLOR_BLUE);
        riv_draw_circle_fill((int)leftX, (int)leftY, 2, RIV_COLOR_BLUE);
    }

    uint32_t transparentOrange = (RIV_COLOR_ORANGE & 0x00FFFFFF) | 0x40000000;
    riv_draw_circle_fill(game.joints[0].x,
                         game.joints[0].y,
                         collisionHeadRadius(),
                         transparentOrange);
}

static void drawDitheredOverlay(uint32_t color) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            if ((x + y) % 2 == 0) {
                riv_draw_point(x, y, color);
            }
        }
    }
}

static void drawGameOver(void) {
    drawDitheredOverlay(RIV_COLOR_BLACK);
    riv_draw_text("GAME OVER",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  TOP_THIRD,
                  TITLE_SCALE,
                  RIV_COLOR_RED);

    char scoreText[32] = "";
    snprintf(scoreText, sizeof(scoreText), "SCORE: %d", game.score);
    riv_draw_text(scoreText,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  MIDDLE,
                  NORMAL_SCALE,
                  RIV_COLOR_LIGHTRED);
}

static void drawGameCompleted(void) {
    uint32_t overlayColor = (RIV_COLOR_DARKSLATE & 0x00FFFFFF) | 0x80000000;
    drawDitheredOverlay(overlayColor);

    char scoreValue[32] = "";
    snprintf(scoreValue, sizeof(scoreValue), "%d", game.score);
    int yPosition = 100;

    riv_draw_text("CONGRATULATIONS!",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  TITLE_SCALE,
                  RIV_COLOR_GOLD);
    yPosition += 30;
    riv_draw_text("ALL LEVELS COMPLETE",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  1.5f,
                  RIV_COLOR_LIGHTGREEN);
    yPosition += 30;
    riv_draw_text("FINAL SCORE:",
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  NORMAL_SCALE,
                  RIV_COLOR_WHITE);
    yPosition += 15;
    riv_draw_text(scoreValue,
                  RIV_SPRITESHEET_FONT_5X7,
                  RIV_CENTER,
                  CENTER_X,
                  yPosition,
                  1.5f,
                  RIV_COLOR_GOLD);
}
