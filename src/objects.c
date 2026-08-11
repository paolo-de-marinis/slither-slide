#include "objects.h"
#include "riv.h"
#include "game.h"
#include "audio.h"
#include "ui_constants.h"
#include "levels.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ObjectState object;

extern GameData game;

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

// Each row stores x1, y1, x2, y2 in the original logo coordinate system.
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

static void drawAppleShape(float rotation);
static void drawCoin(float rotation);
static Point2 rotateCoinPoint(const CoinTransform *transform, float x, float y);
static Point2 transformCoinLogoPoint(const CoinTransform *transform, float x, float y);
static void drawCoinLogo(const CoinTransform *transform);
static bool getCurrentRoomBounds(float *left, float *right, float *top, float *bottom);
static bool
bounceAppleOffRoomBounds(float left, float right, float top, float bottom, float radius);
static void playAppleCollisionSound(bool isCoin);
static void stopAppleIfSlow(void);
static void resolveAppleWallCollision(Wall *wall, float radius, bool isCoin);
static bool cellOverlapsCoinMargin(int x, int y, bool isCoin);
static bool cellOverlapsHud(int x, int y);
static bool cellIsInCorner(int x, int y);
static bool positionIsNearWall(float x, float y, float radius);
static bool positionIsNearBody(float x, float y);
static bool isValidApplePosition(int x, int y, float roomOffsetX, float roomOffsetY, bool isCoin);

void objectsInitialize(void) {
    memset(&object, 0, sizeof(object));
    object.friction = PHYSICS_FRICTION;

    object.wallCount = WALL_SEGMENTS_PER_SIDE * 4;

    float segmentWidth = (SCREEN_WIDTH - 2 * WALL_THICKNESS) / WALL_SEGMENTS_PER_SIDE;
    float segmentHeight = (SCREEN_HEIGHT - 2 * WALL_THICKNESS) / WALL_SEGMENTS_PER_SIDE;

    // Left wall segments
    for (int i = 0; i < WALL_SEGMENTS_PER_SIDE; i++) {
        object.walls[i] = (Wall){.x = 0,
                                 .y = WALL_THICKNESS + (i * segmentHeight),
                                 .width = WALL_THICKNESS,
                                 .height = segmentHeight};
    }

    // Right wall segments
    for (int i = 0; i < WALL_SEGMENTS_PER_SIDE; i++) {
        object.walls[WALL_SEGMENTS_PER_SIDE + i] = (Wall){.x = SCREEN_WIDTH - WALL_THICKNESS,
                                                          .y = WALL_THICKNESS + (i * segmentHeight),
                                                          .width = WALL_THICKNESS,
                                                          .height = segmentHeight};
    }

    // Top wall segments
    for (int i = 0; i < WALL_SEGMENTS_PER_SIDE; i++) {
        object.walls[2 * WALL_SEGMENTS_PER_SIDE + i] =
            (Wall){.x = WALL_THICKNESS + (i * segmentWidth),
                   .y = 0,
                   .width = segmentWidth,
                   .height = WALL_THICKNESS};
    }

    // Bottom wall segments
    for (int i = 0; i < WALL_SEGMENTS_PER_SIDE; i++) {
        object.walls[3 * WALL_SEGMENTS_PER_SIDE + i] =
            (Wall){.x = WALL_THICKNESS + (i * segmentWidth),
                   .y = SCREEN_HEIGHT - WALL_THICKNESS,
                   .width = segmentWidth,
                   .height = WALL_THICKNESS};
    }
}

void drawApple(float rotation) {
    LevelConfig config = getLevelConfig(getCurrentLevel());
    if (game.apples == config.requiredApples - 1) {
        drawCoin(rotation);
        return;
    }
    drawAppleShape(rotation);
}

static void drawAppleShape(float rotation) {
    float screenX = 0.0f;
    float screenY = 0.0f;
    worldToScreen(object.applePhysics.x - TILE_SIZE / 2,
                  object.applePhysics.y - TILE_SIZE / 2,
                  &screenX,
                  &screenY);

    int x = (int)screenX;
    int y = (int)screenY;

    float cosRotation = cosf(rotation);
    float sinRotation = sinf(rotation);

    // Draw shadow
    riv_draw_circle_fill(x + TILE_SIZE / 2 + 1, y + TILE_SIZE / 2 + 1, 6, RIV_COLOR_DARKRED);

    // Apple body
    riv_draw_circle_fill(x + TILE_SIZE / 2, y + TILE_SIZE / 2, 7, RIV_COLOR_RED);

    // Highlight (fixed to upper-left quadrant)
    float highlightAngle = rotation * 0.25f + PI * 1.25f;
    int highlightOffset = 3;
    int highlightX = x + TILE_SIZE / 2 + (int)(highlightOffset * cosf(highlightAngle));
    int highlightY = y + TILE_SIZE / 2 + (int)(highlightOffset * sinf(highlightAngle));
    riv_draw_circle_fill(highlightX, highlightY, 3, RIV_COLOR_WHITE);

    // Stem (shortened)
    int stemStartX = x + TILE_SIZE / 2;
    int stemStartY = y + TILE_SIZE / 2;
    int stemEndX = stemStartX + (int)(3 * sinf(rotation));
    int stemEndY = stemStartY - 5;
    riv_draw_line(stemStartX, stemStartY - 2, stemEndX, stemEndY, RIV_COLOR_BROWN);

    // Leaves
    int leafOffset = 2;
    int leafOneX = stemEndX + (int)(leafOffset * cosRotation);
    int leafOneY = stemEndY + (int)(leafOffset * sinRotation);
    riv_draw_circle_fill(leafOneX, leafOneY, 2, RIV_COLOR_GREEN);

    int leafTwoX = stemEndX + (int)(leafOffset * cosf(rotation + PI / 2.0f));
    int leafTwoY = stemEndY + (int)(leafOffset * sinf(rotation + PI / 2.0f));
    riv_draw_circle_fill(leafTwoX, leafTwoY, 2, RIV_COLOR_DARKGREEN);
}

static void drawCoin(float rotation) {
    float screenX = 0.0f;
    float screenY = 0.0f;
    worldToScreen(object.applePhysics.x - TILE_SIZE / 2,
                  object.applePhysics.y - TILE_SIZE / 2,
                  &screenX,
                  &screenY);

    int x = (int)screenX;
    int y = (int)screenY;
    int centerX = x + TILE_SIZE / 2;
    int centerY = y + TILE_SIZE / 2;

    const int BASE_COIN_RADIUS = 16;
    float pulse = sinf(rotation * 2.0f) * 0.05f + 1.0f;
    int coinRadius = (int)(BASE_COIN_RADIUS * pulse);
    float swingAngle = sinf(rotation * 1.5f) * (PI / 12.0f);
    CoinTransform transform = {.centerX = (float)centerX,
                               .centerY = (float)centerY,
                               .cosine = cosf(swingAngle),
                               .sine = sinf(swingAngle),
                               .logoScale = (coinRadius * 0.45f) / 25.0f};

    uint32_t coinColor = (getCurrentLevel() == MAX_LEVEL) ? RIV_COLOR_GOLD : RIV_COLOR_LIGHTGREY;
    uint32_t coinShadowColor =
        (getCurrentLevel() == MAX_LEVEL) ? RIV_COLOR_DARKBROWN : RIV_COLOR_GREY;

    float shadowOffset = 2.0f + pulse;
    Point2 shadow = rotateCoinPoint(&transform, centerX + shadowOffset, centerY + shadowOffset);
    riv_draw_circle_fill(shadow.x, shadow.y, coinRadius - 1, coinShadowColor);

    Point2 center = rotateCoinPoint(&transform, centerX, centerY);
    riv_draw_circle_fill(center.x, center.y, coinRadius, coinColor);

    float shineAngle = PI * 1.25f;
    float shineDistance = coinRadius * 0.4f;
    float shineX = centerX + shineDistance * cosf(shineAngle);
    float shineY = centerY + shineDistance * sinf(shineAngle);
    Point2 shine = rotateCoinPoint(&transform, shineX, shineY);
    riv_draw_circle_fill(shine.x, shine.y, coinRadius * 0.2f, RIV_COLOR_WHITE);

    drawCoinLogo(&transform);
}

static Point2 rotateCoinPoint(const CoinTransform *transform, float x, float y) {
    float deltaX = x - transform->centerX;
    float deltaY = y - transform->centerY;
    Point2 result = {
        .x = transform->centerX + (deltaX * transform->cosine - deltaY * transform->sine),
        .y = transform->centerY + (deltaX * transform->sine + deltaY * transform->cosine)};
    return result;
}

static Point2 transformCoinLogoPoint(const CoinTransform *transform, float x, float y) {
    float centeredX = (x - 49.3f / 2.0f) * transform->logoScale + 1.0f;
    float centeredY = (y - 50.1f / 2.0f) * transform->logoScale + 1.0f;
    return rotateCoinPoint(
        transform, transform->centerX + centeredX, transform->centerY + centeredY);
}

static void drawCoinLogo(const CoinTransform *transform) {
    int segmentCount = (int)(sizeof(COIN_LOGO_SEGMENTS) / sizeof(COIN_LOGO_SEGMENTS[0]));
    for (int i = 0; i < segmentCount; i++) {
        Point2 start =
            transformCoinLogoPoint(transform, COIN_LOGO_SEGMENTS[i][0], COIN_LOGO_SEGMENTS[i][1]);
        Point2 end =
            transformCoinLogoPoint(transform, COIN_LOGO_SEGMENTS[i][2], COIN_LOGO_SEGMENTS[i][3]);
        riv_draw_line(start.x, start.y, end.x, end.y, RIV_COLOR_BLACK);
    }
}

void drawWalls(void) {
    for (int i = 0; i < object.wallCount; i++) {
        Wall *wall = &object.walls[i];

        // Skip walls with zero dimensions (removed walls)
        if (wall->width == 0 || wall->height == 0) {
            continue;
        }

        // Convert wall position to screen coordinates
        float screenX = 0.0f;
        float screenY = 0.0f;
        worldToScreen(wall->x, wall->y, &screenX, &screenY);

        // Add a small gap between segments (1 pixel)
        riv_draw_rect_fill(
            screenX + 1, screenY + 1, wall->width - 2, wall->height - 2, RIV_COLOR_DARKGREEN);
    }
}

void updateApplePhysics(void) {
    if (!object.applePhysics.active) {
        return;
    }

    object.applePhysics.x += object.applePhysics.vx;
    object.applePhysics.y += object.applePhysics.vy;
    object.applePhysics.vx *= object.friction;
    object.applePhysics.vy *= object.friction;

    LevelConfig config = getLevelConfig(getCurrentLevel());
    bool isCoin = (game.apples == config.requiredApples - 1);

    float roomLeft = 0.0f;
    float roomRight = 0.0f;
    float roomTop = 0.0f;
    float roomBottom = 0.0f;
    if (!getCurrentRoomBounds(&roomLeft, &roomRight, &roomTop, &roomBottom)) {
        object.applePhysics.active = false;
        return;
    }

    float radius = 7.0f;
    bool didBounce = bounceAppleOffRoomBounds(roomLeft, roomRight, roomTop, roomBottom, radius);
    if (didBounce && (fabsf(object.applePhysics.vx) > MIN_VELOCITY ||
                      fabsf(object.applePhysics.vy) > MIN_VELOCITY)) {
        playAppleCollisionSound(isCoin);
    }

    stopAppleIfSlow();
    object.applePosition.x = (int)(object.applePhysics.x / TILE_SIZE);
    object.applePosition.y = (int)(object.applePhysics.y / TILE_SIZE);

    for (int i = 0; i < object.wallCount; i++) {
        resolveAppleWallCollision(&object.walls[i], radius, isCoin);
    }
}

static bool getCurrentRoomBounds(float *left, float *right, float *top, float *bottom) {
    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(getCurrentLevel(), &roomX, &roomY)) {
        return false;
    }

    *left = roomX * ROOM_WIDTH;
    *right = (roomX + 1) * ROOM_WIDTH;
    *top = roomY * ROOM_HEIGHT;
    *bottom = (roomY + 1) * ROOM_HEIGHT;
    return true;
}

static bool
bounceAppleOffRoomBounds(float left, float right, float top, float bottom, float radius) {
    bool didBounce = false;
    if (object.applePhysics.x < left + WALL_THICKNESS + radius) {
        object.applePhysics.x = left + WALL_THICKNESS + radius;
        object.applePhysics.vx *= -0.8f;
        didBounce = true;
    }
    if (object.applePhysics.x > right - WALL_THICKNESS - radius) {
        object.applePhysics.x = right - WALL_THICKNESS - radius;
        object.applePhysics.vx *= -0.8f;
        didBounce = true;
    }
    if (object.applePhysics.y < top + WALL_THICKNESS + radius) {
        object.applePhysics.y = top + WALL_THICKNESS + radius;
        object.applePhysics.vy *= -0.8f;
        didBounce = true;
    }
    if (object.applePhysics.y > bottom - WALL_THICKNESS - radius) {
        object.applePhysics.y = bottom - WALL_THICKNESS - radius;
        object.applePhysics.vy *= -0.8f;
        didBounce = true;
    }
    return didBounce;
}

static void playAppleCollisionSound(bool isCoin) {
    if (isCoin) {
        playCoinBounceSound();
    } else {
        playAppleBounceSound();
    }
}

static void stopAppleIfSlow(void) {
    if (fabsf(object.applePhysics.vx) < MIN_VELOCITY &&
        fabsf(object.applePhysics.vy) < MIN_VELOCITY) {
        object.applePhysics.active = false;
        object.applePhysics.vx = 0.0f;
        object.applePhysics.vy = 0.0f;
    }
}

static void resolveAppleWallCollision(Wall *wall, float radius, bool isCoin) {
    float closestX = fmaxf(wall->x, fminf(object.applePhysics.x, wall->x + wall->width));
    float closestY = fmaxf(wall->y, fminf(object.applePhysics.y, wall->y + wall->height));
    float deltaX = object.applePhysics.x - closestX;
    float deltaY = object.applePhysics.y - closestY;
    float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
    if (distance >= radius) {
        return;
    }

    float normalX = 0.0f;
    float normalY = 0.0f;
    if (distance > NORMAL_EPSILON) {
        normalX = deltaX / distance;
        normalY = deltaY / distance;
    } else if (wall->width < wall->height) {
        float wallCenter = wall->x + wall->width / 2.0f;
        normalX = object.applePhysics.x < wallCenter ? -1.0f : 1.0f;
        closestX = normalX < 0.0f ? wall->x : wall->x + wall->width;
    } else {
        float wallCenter = wall->y + wall->height / 2.0f;
        normalY = object.applePhysics.y < wallCenter ? -1.0f : 1.0f;
        closestY = normalY < 0.0f ? wall->y : wall->y + wall->height;
    }

    object.applePhysics.x = closestX + normalX * radius;
    object.applePhysics.y = closestY + normalY * radius;

    float velocityAlongNormal = object.applePhysics.vx * normalX + object.applePhysics.vy * normalY;
    object.applePhysics.vx =
        (-2.0f * velocityAlongNormal * normalX + object.applePhysics.vx) * 0.8f;
    object.applePhysics.vy =
        (-2.0f * velocityAlongNormal * normalY + object.applePhysics.vy) * 0.8f;
    playAppleCollisionSound(isCoin);
}

static bool cellOverlapsCoinMargin(int x, int y, bool isCoin) {
    return isCoin && (x <= 3 || x >= MAP_SIZE - 4 || y <= 3 || y >= MAP_SIZE - 4);
}

static bool cellOverlapsHud(int x, int y) {
    int scoreX = (SCREEN_WIDTH - WALL_THICKNESS - 80) / TILE_SIZE;
    int levelX = WALL_THICKNESS / TILE_SIZE;
    int uiY = WALL_THICKNESS / TILE_SIZE;
    int uiHeight = (TOP_FIFTH - WALL_THICKNESS) / TILE_SIZE;

    if (y > uiY + uiHeight) {
        return false;
    }
    if (x >= scoreX && x < scoreX + 12) {
        return true;
    }
    return x <= levelX + 8;
}

static bool cellIsInCorner(int x, int y) {
    const int CORNER_BUFFER = 2;
    bool nearLeft = x <= CORNER_BUFFER;
    bool nearRight = x >= MAP_SIZE - CORNER_BUFFER - 1;
    bool nearTop = y <= CORNER_BUFFER;
    bool nearBottom = y >= MAP_SIZE - CORNER_BUFFER - 1;
    return (nearLeft || nearRight) && (nearTop || nearBottom);
}

static bool positionIsNearWall(float x, float y, float radius) {
    for (int i = 0; i < object.wallCount; i++) {
        Wall *wall = &object.walls[i];
        if (wall->width == 0 || wall->height == 0) {
            continue;
        }

        float closestX = fmaxf(wall->x, fminf(x, wall->x + wall->width));
        float closestY = fmaxf(wall->y, fminf(y, wall->y + wall->height));
        float deltaX = x - closestX;
        float deltaY = y - closestY;
        float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared < (radius + 2.0f) * (radius + 2.0f)) {
            return true;
        }
    }
    return false;
}

static bool positionIsNearBody(float x, float y) {
    for (int i = 0; i < game.jointCount; i++) {
        float deltaX = x - game.joints[i].x;
        float deltaY = y - game.joints[i].y;
        float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared < TILE_SIZE * TILE_SIZE * 1.5f) {
            return true;
        }
    }
    return false;
}

static bool isValidApplePosition(int x, int y, float roomOffsetX, float roomOffsetY, bool isCoin) {
    if (cellOverlapsCoinMargin(x, y, isCoin) || cellOverlapsHud(x, y) || cellIsInCorner(x, y)) {
        return false;
    }

    float centerX = roomOffsetX + x * TILE_SIZE + TILE_SIZE / 2;
    float centerY = roomOffsetY + y * TILE_SIZE + TILE_SIZE / 2;
    float radius = 7.0f;
    if (isCoin) {
        radius = 16.0f;
    }
    return !positionIsNearWall(centerX, centerY, radius) && !positionIsNearBody(centerX, centerY);
}

bool spawnApple(void) {
    riv_vec2i validPositions[MAP_SIZE * MAP_SIZE];
    int validCount = 0;

    LevelConfig config = getLevelConfig(getCurrentLevel());
    bool isCoin = (game.apples == config.requiredApples - 1);

    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(getCurrentLevel(), &roomX, &roomY)) {
        return false;
    }
    float roomOffsetX = roomX * ROOM_WIDTH;
    float roomOffsetY = roomY * ROOM_HEIGHT;

    // Scan order must stay stable because the random index selects from this array.
    for (int y = 1; y < MAP_SIZE - 1; y++) {
        for (int x = 1; x < MAP_SIZE - 1; x++) {
            if (isValidApplePosition(x, y, roomOffsetX, roomOffsetY, isCoin) &&
                validCount < MAP_SIZE * MAP_SIZE) {
                validPositions[validCount] = (riv_vec2i){x, y};
                validCount++;
            }
        }
    }

    if (validCount == 0) {
        return false;
    }

    int selected = riv_rand_uint(validCount - 1);
    object.applePosition = validPositions[selected];

    object.applePhysics.x = roomOffsetX + object.applePosition.x * TILE_SIZE + TILE_SIZE / 2;
    object.applePhysics.y = roomOffsetY + object.applePosition.y * TILE_SIZE + TILE_SIZE / 2;
    object.applePhysics.vx = 0.0f;
    object.applePhysics.vy = 0.0f;
    object.applePhysics.active = false;
    object.friction = PHYSICS_FRICTION;

    return true;
}
