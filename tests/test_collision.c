#include "collision.h"
#include "room_layout.h"
#include "walls.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

GameData game;
static int testLevel = 5;

int getCurrentLevel(void) {
    return testLevel;
}

bool canTransitionToLevel(int level) {
    (void)level;
    return false;
}

float snakeBodyWidth(int segmentIndex) {
    (void)segmentIndex;
    return 8.0f;
}

void worldToScreen(float worldX, float worldY, float *screenX, float *screenY) {
    *screenX = worldX;
    *screenY = worldY;
}

void riv_draw_rect_fill(int64_t x,
                        int64_t y,
                        int64_t width,
                        int64_t height,
                        uint32_t color) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}

uint64_t riv_printf(const char *format, ...) {
    (void)format;
    return 0;
}

int main(void) {
    const float historicalRadius = 8.0f * 0.67f;
    assert(fabsf(collisionHeadRadius() - historicalRadius) < 0.0001f);

    int roomX = 0;
    int roomY = 0;
    assert(getRoomPosition(5, &roomX, &roomY));

    float roomOffsetX = roomX * ROOM_WIDTH;
    float roomOffsetY = roomY * ROOM_HEIGHT;
    wallsBeginRoom(roomOffsetX, roomOffsetY);
    assert(wallsAdd((Wall){roomOffsetX + 3 * ROOM_WIDTH / 4 - 10,
                           roomOffsetY + ROOM_HEIGHT / 4,
                           20,
                           ROOM_HEIGHT / 2}));

    riv_vec2i historicalClearance = {29, 95};
    float headX = historicalClearance.x * TILE_SIZE + TILE_SIZE / 2.0f;
    float headY = historicalClearance.y * TILE_SIZE + TILE_SIZE / 2.0f;
    const Wall *obstacle = wallsGet(wallsGetCount() - 1);
    assert(obstacle != NULL);
    assert(!wallCircleContact(obstacle, headX, headY, historicalRadius, NULL));
    assert(wallCircleContact(obstacle, headX, headY, 6.0f, NULL));

    GameData state = {0};
    assert(!collisionAtNextHead(&state, historicalClearance));
    assert(collisionAtNextHead(&state, (riv_vec2i){30, 95}));

    testLevel = 2;
    assert(getRoomPosition(testLevel, &roomX, &roomY));
    roomOffsetX = roomX * ROOM_WIDTH;
    roomOffsetY = roomY * ROOM_HEIGHT;
    wallsBeginRoom(roomOffsetX, roomOffsetY);
    assert(wallsAdd((Wall){roomOffsetX + ROOM_WIDTH / 2 - 20,
                           roomOffsetY + ROOM_HEIGHT / 2 - 20,
                           40,
                           40}));

    riv_vec2i visibleCornerClearance = {60, 25};
    headX = visibleCornerClearance.x * TILE_SIZE + TILE_SIZE / 2.0f;
    headY = visibleCornerClearance.y * TILE_SIZE + TILE_SIZE / 2.0f;
    obstacle = wallsGet(wallsGetCount() - 1);
    assert(obstacle != NULL);
    assert(wallCircleContact(obstacle, headX, headY, historicalRadius, NULL));
    Wall drawnObstacle = {obstacle->x + WALL_DRAW_INSET,
                          obstacle->y + WALL_DRAW_INSET,
                          obstacle->width - 2 * WALL_DRAW_INSET,
                          obstacle->height - 2 * WALL_DRAW_INSET};
    assert(!wallCircleContact(&drawnObstacle, headX, headY, historicalRadius, NULL));
    assert(!collisionAtNextHead(&state, visibleCornerClearance));
    assert(collisionAtNextHead(&state, (riv_vec2i){60, 24}));

    puts("collision radius, level-2 cube and level-5 clearance: ok");
    return 0;
}
