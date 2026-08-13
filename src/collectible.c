#include "collectible.h"

#include "audio.h"
#include "collectible_render.h"
#include "levels.h"
#include "snake_char.h"
#include "ui_constants.h"
#include "walls.h"

#include <math.h>
#include <string.h>

#define PHYSICS_FRICTION 0.95f
#define COLLISION_FORCE 1.0f
#define MIN_VELOCITY 0.1f
#define NORMAL_EPSILON 0.0001f
#define APPLE_RADIUS 7.0f
#define COIN_RADIUS 16.0f

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    bool moving;
} PhysicsObject;

typedef struct {
    bool present;
    PhysicsObject physics;
} CollectibleState;

static CollectibleState collectible;

static bool currentCollectibleIsCoin(const GameData *state);
static bool getCurrentRoomBounds(float *left, float *right, float *top, float *bottom);
static bool bounceOffRoomBounds(float left, float right, float top, float bottom, float radius);
static void playCollisionSound(bool isCoin);
static void stopIfSlow(void);
static void resolveWallCollision(const Wall *wall, float radius, bool isCoin);
static bool isValidCell(const GameData *state,
                        int x,
                        int y,
                        float roomOffsetX,
                        float roomOffsetY,
                        bool isCoin);

void collectibleInitialize(void) {
    memset(&collectible, 0, sizeof(collectible));
}

void collectibleHide(void) {
    collectible.present = false;
    collectible.physics.moving = false;
    collectible.physics.vx = 0.0f;
    collectible.physics.vy = 0.0f;
}

static bool currentCollectibleIsCoin(const GameData *state) {
    LevelConfig config;
    int level = getCurrentLevel();
    return getLevelConfig(level, &config) && state->levelItems[level - 1] == config.requiredItems - 1;
}

void collectibleDraw(const GameData *state, float rotation) {
    if (!collectible.present) {
        return;
    }
    collectibleRender(collectible.physics.x,
                      collectible.physics.y,
                      currentCollectibleIsCoin(state),
                      getCurrentLevel() == MAX_LEVEL,
                      rotation);
}

bool collectibleTouchesHead(const GameData *state, float headRadius) {
    if (!collectible.present) {
        return false;
    }
    float headX = state->headPosition.x * TILE_SIZE + TILE_SIZE / 2.0f;
    float headY = state->headPosition.y * TILE_SIZE + TILE_SIZE / 2.0f;
    float deltaX = headX - collectible.physics.x;
    float deltaY = headY - collectible.physics.y;
    float collectDistance = headRadius + APPLE_RADIUS;
    return deltaX * deltaX + deltaY * deltaY < collectDistance * collectDistance;
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

static bool bounceOffRoomBounds(float left, float right, float top, float bottom, float radius) {
    bool bounced = false;
    if (collectible.physics.x < left + WALL_THICKNESS + radius) {
        collectible.physics.x = left + WALL_THICKNESS + radius;
        collectible.physics.vx *= -0.8f;
        bounced = true;
    }
    if (collectible.physics.x > right - WALL_THICKNESS - radius) {
        collectible.physics.x = right - WALL_THICKNESS - radius;
        collectible.physics.vx *= -0.8f;
        bounced = true;
    }
    if (collectible.physics.y < top + WALL_THICKNESS + radius) {
        collectible.physics.y = top + WALL_THICKNESS + radius;
        collectible.physics.vy *= -0.8f;
        bounced = true;
    }
    if (collectible.physics.y > bottom - WALL_THICKNESS - radius) {
        collectible.physics.y = bottom - WALL_THICKNESS - radius;
        collectible.physics.vy *= -0.8f;
        bounced = true;
    }
    return bounced;
}

static void playCollisionSound(bool isCoin) {
    if (isCoin) {
        playCoinBounceSound();
    } else {
        playAppleBounceSound();
    }
}

static void stopIfSlow(void) {
    if (fabsf(collectible.physics.vx) < MIN_VELOCITY &&
        fabsf(collectible.physics.vy) < MIN_VELOCITY) {
        collectible.physics.moving = false;
        collectible.physics.vx = 0.0f;
        collectible.physics.vy = 0.0f;
    }
}

static void resolveWallCollision(const Wall *wall, float radius, bool isCoin) {
    WallContact contact;
    if (!wallCircleContact(wall,
                           collectible.physics.x,
                           collectible.physics.y,
                           radius,
                           &contact)) {
        return;
    }

    float normalX = 0.0f;
    float normalY = 0.0f;
    if (contact.distance > NORMAL_EPSILON) {
        normalX = contact.deltaX / contact.distance;
        normalY = contact.deltaY / contact.distance;
    } else if (wall->width < wall->height) {
        float wallCenter = wall->x + wall->width / 2.0f;
        normalX = collectible.physics.x < wallCenter ? -1.0f : 1.0f;
        contact.closestX = normalX < 0.0f ? wall->x : wall->x + wall->width;
    } else {
        float wallCenter = wall->y + wall->height / 2.0f;
        normalY = collectible.physics.y < wallCenter ? -1.0f : 1.0f;
        contact.closestY = normalY < 0.0f ? wall->y : wall->y + wall->height;
    }

    collectible.physics.x = contact.closestX + normalX * radius;
    collectible.physics.y = contact.closestY + normalY * radius;

    float normalVelocity = collectible.physics.vx * normalX + collectible.physics.vy * normalY;
    collectible.physics.vx = (collectible.physics.vx - 2.0f * normalVelocity * normalX) * 0.8f;
    collectible.physics.vy = (collectible.physics.vy - 2.0f * normalVelocity * normalY) * 0.8f;
    playCollisionSound(isCoin);
}

void collectibleUpdatePhysics(const GameData *state) {
    if (!collectible.present || !collectible.physics.moving) {
        return;
    }

    collectible.physics.x += collectible.physics.vx;
    collectible.physics.y += collectible.physics.vy;
    collectible.physics.vx *= PHYSICS_FRICTION;
    collectible.physics.vy *= PHYSICS_FRICTION;

    float left = 0.0f;
    float right = 0.0f;
    float top = 0.0f;
    float bottom = 0.0f;
    if (!getCurrentRoomBounds(&left, &right, &top, &bottom)) {
        collectibleHide();
        return;
    }

    bool isCoin = currentCollectibleIsCoin(state);
    if (bounceOffRoomBounds(left, right, top, bottom, APPLE_RADIUS) &&
        (fabsf(collectible.physics.vx) > MIN_VELOCITY ||
         fabsf(collectible.physics.vy) > MIN_VELOCITY)) {
        playCollisionSound(isCoin);
    }

    stopIfSlow();
    for (int index = 0; index < wallsGetCount(); index++) {
        resolveWallCollision(wallsGet(index), APPLE_RADIUS, isCoin);
    }
}

void collectiblePushFromBody(const GameData *state) {
    if (!collectible.present || collectible.physics.moving) {
        return;
    }

    const float sideOffsets[] = {-PI / 2.0f, PI / 2.0f};
    for (int index = 2; index < state->jointCount - 1; index++) {
        float bodyRadius = getSnakeBodyWidth(index) * 0.67f;
        for (int side = 0; side < 2; side++) {
            float profileX = 0.0f;
            float profileY = 0.0f;
            getOffsetPosition(state->joints,
                              index,
                              sideOffsets[side],
                              &profileX,
                              &profileY);

            float deltaX = collectible.physics.x - profileX;
            float deltaY = collectible.physics.y - profileY;
            float distanceSquared = deltaX * deltaX + deltaY * deltaY;
            float collisionRadius = bodyRadius + APPLE_RADIUS;
            if (distanceSquared >= collisionRadius * collisionRadius) {
                continue;
            }

            float normalX = 0.0f;
            float normalY = 0.0f;
            if (distanceSquared > NORMAL_EPSILON * NORMAL_EPSILON) {
                float distance = sqrtf(distanceSquared);
                normalX = deltaX / distance;
                normalY = deltaY / distance;
            } else {
                normalX = cosf(state->joints[index].angle + sideOffsets[side]);
                normalY = sinf(state->joints[index].angle + sideOffsets[side]);
            }

            collectible.physics.vx = normalX * COLLISION_FORCE;
            collectible.physics.vy = normalY * COLLISION_FORCE;
            collectible.physics.moving = true;
            playCollisionSound(currentCollectibleIsCoin(state));
            return;
        }
    }
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
    return (x >= scoreX && x < scoreX + 12) || x <= levelX + 8;
}

static bool cellIsInCorner(int x, int y) {
    const int cornerBuffer = 2;
    bool nearLeft = x <= cornerBuffer;
    bool nearRight = x >= MAP_SIZE - cornerBuffer - 1;
    bool nearTop = y <= cornerBuffer;
    bool nearBottom = y >= MAP_SIZE - cornerBuffer - 1;
    return (nearLeft || nearRight) && (nearTop || nearBottom);
}

static bool positionIsNearBody(const GameData *state, float x, float y) {
    for (int index = 0; index < state->jointCount; index++) {
        float deltaX = x - state->joints[index].x;
        float deltaY = y - state->joints[index].y;
        if (deltaX * deltaX + deltaY * deltaY < TILE_SIZE * TILE_SIZE * 1.5f) {
            return true;
        }
    }
    return false;
}

static bool isValidCell(const GameData *state,
                        int x,
                        int y,
                        float roomOffsetX,
                        float roomOffsetY,
                        bool isCoin) {
    if (cellOverlapsCoinMargin(x, y, isCoin) || cellOverlapsHud(x, y) || cellIsInCorner(x, y)) {
        return false;
    }

    float centerX = roomOffsetX + x * TILE_SIZE + TILE_SIZE / 2.0f;
    float centerY = roomOffsetY + y * TILE_SIZE + TILE_SIZE / 2.0f;
    float radius = isCoin ? COIN_RADIUS : APPLE_RADIUS;
    return wallsPositionIsClear(centerX, centerY, radius, 2.0f) &&
           !positionIsNearBody(state, centerX, centerY);
}

bool collectibleSpawn(const GameData *state) {
    riv_vec2i validPositions[MAP_SIZE * MAP_SIZE];
    int validCount = 0;
    bool isCoin = currentCollectibleIsCoin(state);

    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(getCurrentLevel(), &roomX, &roomY)) {
        return false;
    }
    float roomOffsetX = roomX * ROOM_WIDTH;
    float roomOffsetY = roomY * ROOM_HEIGHT;

    /* Scan order is part of deterministic RIVES entropy consumption. */
    for (int y = 1; y < MAP_SIZE - 1; y++) {
        for (int x = 1; x < MAP_SIZE - 1; x++) {
            if (isValidCell(state, x, y, roomOffsetX, roomOffsetY, isCoin)) {
                validPositions[validCount] = (riv_vec2i){x, y};
                validCount++;
            }
        }
    }

    if (validCount == 0) {
        collectibleHide();
        return false;
    }

    int selected = riv_rand_uint(validCount - 1);
    riv_vec2i selectedCell = validPositions[selected];
    collectible.physics.x = roomOffsetX + selectedCell.x * TILE_SIZE + TILE_SIZE / 2.0f;
    collectible.physics.y = roomOffsetY + selectedCell.y * TILE_SIZE + TILE_SIZE / 2.0f;
    collectible.physics.vx = 0.0f;
    collectible.physics.vy = 0.0f;
    collectible.physics.moving = false;
    collectible.present = true;
    return true;
}
