#include "collectible_render.h"

#include "camera.h"
#include "game_state.h"
#include "riv.h"

#include <math.h>
#include <stddef.h>

#define COIN_RENDER_RADIUS 16.0f

typedef struct {
    float x;
    float y;
} Point2;

typedef struct {
    float centerX;
    float centerY;
    float cosine;
    float sine;
    float logoScale;
} CoinTransform;

static const float COIN_LOGO_SEGMENTS[][4] = {{11.4f, 17.0f, 26.0f, 1.0f},
                                              {26.0f, 1.0f, 32.0f, 17.0f},
                                              {31.7f, 39.8f, 23.2f, 49.1f},
                                              {23.2f, 49.1f, 17.2f, 33.1f},
                                              {37.9f, 33.0f, 31.7f, 39.7f},
                                              {31.9f, 17.0f, 42.3f, 17.0f},
                                              {42.3f, 17.0f, 48.3f, 33.0f},
                                              {48.3f, 33.0f, 27.7f, 33.0f},
                                              {27.7f, 33.0f, 21.7f, 17.0f},
                                              {21.7f, 17.0f, 1.0f, 17.0f},
                                              {1.0f, 17.0f, 7.0f, 33.0f},
                                              {7.0f, 33.0f, 17.3f, 33.0f},
                                              {31.9f, 17.0f, 17.2f, 33.0f}};

static void drawApple(float screenX, float screenY, float rotation);
static void drawCoin(float screenX, float screenY, bool isFinalLevel, float rotation);
static Point2 rotateCoinPoint(const CoinTransform *transform, float x, float y);
static Point2 transformCoinLogoPoint(const CoinTransform *transform, float x, float y);
static void drawCoinLogo(const CoinTransform *transform);

void collectibleRender(float worldX,
                       float worldY,
                       bool isCoin,
                       bool isFinalLevel,
                       float rotation) {
    float screenX = 0.0f;
    float screenY = 0.0f;
    worldToScreen(worldX - TILE_SIZE / 2.0f,
                  worldY - TILE_SIZE / 2.0f,
                  &screenX,
                  &screenY);
    if (isCoin) {
        drawCoin(screenX, screenY, isFinalLevel, rotation);
    } else {
        drawApple(screenX, screenY, rotation);
    }
}

static void drawApple(float screenX, float screenY, float rotation) {
    int x = (int)screenX;
    int y = (int)screenY;
    float cosRotation = cosf(rotation);
    float sinRotation = sinf(rotation);

    riv_draw_circle_fill(x + TILE_SIZE / 2 + 1,
                         y + TILE_SIZE / 2 + 1,
                         6,
                         RIV_COLOR_DARKRED);
    riv_draw_circle_fill(x + TILE_SIZE / 2, y + TILE_SIZE / 2, 7, RIV_COLOR_RED);

    float highlightAngle = rotation * 0.25f + PI * 1.25f;
    int highlightOffset = 3;
    int highlightX = x + TILE_SIZE / 2 + (int)(highlightOffset * cosf(highlightAngle));
    int highlightY = y + TILE_SIZE / 2 + (int)(highlightOffset * sinf(highlightAngle));
    riv_draw_circle_fill(highlightX, highlightY, 3, RIV_COLOR_WHITE);

    int stemStartX = x + TILE_SIZE / 2;
    int stemStartY = y + TILE_SIZE / 2;
    int stemEndX = stemStartX + (int)(3 * sinf(rotation));
    int stemEndY = stemStartY - 5;
    riv_draw_line(stemStartX, stemStartY - 2, stemEndX, stemEndY, RIV_COLOR_BROWN);

    int leafOffset = 2;
    int leafOneX = stemEndX + (int)(leafOffset * cosRotation);
    int leafOneY = stemEndY + (int)(leafOffset * sinRotation);
    riv_draw_circle_fill(leafOneX, leafOneY, 2, RIV_COLOR_GREEN);

    int leafTwoX = stemEndX + (int)(leafOffset * cosf(rotation + PI / 2.0f));
    int leafTwoY = stemEndY + (int)(leafOffset * sinf(rotation + PI / 2.0f));
    riv_draw_circle_fill(leafTwoX, leafTwoY, 2, RIV_COLOR_DARKGREEN);
}

static void drawCoin(float screenX, float screenY, bool isFinalLevel, float rotation) {
    int centerX = (int)screenX + TILE_SIZE / 2;
    int centerY = (int)screenY + TILE_SIZE / 2;
    float pulse = sinf(rotation * 2.0f) * 0.05f + 1.0f;
    int coinRadius = (int)(COIN_RENDER_RADIUS * pulse);
    float swingAngle = sinf(rotation * 1.5f) * (PI / 12.0f);
    CoinTransform transform = {.centerX = (float)centerX,
                               .centerY = (float)centerY,
                               .cosine = cosf(swingAngle),
                               .sine = sinf(swingAngle),
                               .logoScale = (coinRadius * 0.45f) / 25.0f};

    uint32_t coinColor = isFinalLevel ? RIV_COLOR_GOLD : RIV_COLOR_LIGHTGREY;
    uint32_t shadowColor = isFinalLevel ? RIV_COLOR_DARKBROWN : RIV_COLOR_GREY;
    float shadowOffset = 2.0f + pulse;
    Point2 shadow = rotateCoinPoint(&transform, centerX + shadowOffset, centerY + shadowOffset);
    riv_draw_circle_fill(shadow.x, shadow.y, coinRadius - 1, shadowColor);

    Point2 center = rotateCoinPoint(&transform, centerX, centerY);
    riv_draw_circle_fill(center.x, center.y, coinRadius, coinColor);

    float shineAngle = PI * 1.25f;
    float shineDistance = coinRadius * 0.4f;
    Point2 shine = rotateCoinPoint(&transform,
                                   centerX + shineDistance * cosf(shineAngle),
                                   centerY + shineDistance * sinf(shineAngle));
    riv_draw_circle_fill(shine.x, shine.y, coinRadius * 0.2f, RIV_COLOR_WHITE);
    drawCoinLogo(&transform);
}

static Point2 rotateCoinPoint(const CoinTransform *transform, float x, float y) {
    float deltaX = x - transform->centerX;
    float deltaY = y - transform->centerY;
    return (Point2){
        .x = transform->centerX + deltaX * transform->cosine - deltaY * transform->sine,
        .y = transform->centerY + deltaX * transform->sine + deltaY * transform->cosine
    };
}

static Point2 transformCoinLogoPoint(const CoinTransform *transform, float x, float y) {
    float centeredX = (x - 49.3f / 2.0f) * transform->logoScale + 1.0f;
    float centeredY = (y - 50.1f / 2.0f) * transform->logoScale + 1.0f;
    return rotateCoinPoint(transform,
                           transform->centerX + centeredX,
                           transform->centerY + centeredY);
}

static void drawCoinLogo(const CoinTransform *transform) {
    size_t segmentCount = sizeof(COIN_LOGO_SEGMENTS) / sizeof(COIN_LOGO_SEGMENTS[0]);
    for (size_t index = 0; index < segmentCount; index++) {
        Point2 start = transformCoinLogoPoint(transform,
                                              COIN_LOGO_SEGMENTS[index][0],
                                              COIN_LOGO_SEGMENTS[index][1]);
        Point2 end = transformCoinLogoPoint(transform,
                                            COIN_LOGO_SEGMENTS[index][2],
                                            COIN_LOGO_SEGMENTS[index][3]);
        riv_draw_line(start.x, start.y, end.x, end.y, RIV_COLOR_BLACK);
    }
}
