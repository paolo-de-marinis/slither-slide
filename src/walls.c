#include "walls.h"

#include "camera.h"
#include "game_state.h"
#include "riv.h"
#include "ui_constants.h"

#include <math.h>
#include <string.h>

static struct {
    Wall entries[MAX_WALLS];
    int count;
} wallState;

static bool wallIsPresent(const Wall *wall) {
    return wall != NULL && wall->width > 0.0f && wall->height > 0.0f;
}

void wallsInitializeMenu(void) {
    memset(&wallState, 0, sizeof(wallState));

    float segmentWidth = (SCREEN_WIDTH - 2 * WALL_THICKNESS) / (float)WALL_SEGMENTS_PER_SIDE;
    float segmentHeight =
        (SCREEN_HEIGHT - 2 * WALL_THICKNESS) / (float)WALL_SEGMENTS_PER_SIDE;

    for (int segment = 0; segment < WALL_SEGMENTS_PER_SIDE; segment++) {
        wallState.entries[segment] =
            (Wall){0.0f,
                   WALL_THICKNESS + segment * segmentHeight,
                   WALL_THICKNESS,
                   segmentHeight};
        wallState.entries[WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){SCREEN_WIDTH - WALL_THICKNESS,
                   WALL_THICKNESS + segment * segmentHeight,
                   WALL_THICKNESS,
                   segmentHeight};
        wallState.entries[2 * WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){WALL_THICKNESS + segment * segmentWidth,
                   0.0f,
                   segmentWidth,
                   WALL_THICKNESS};
        wallState.entries[3 * WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){WALL_THICKNESS + segment * segmentWidth,
                   SCREEN_HEIGHT - WALL_THICKNESS,
                   segmentWidth,
                   WALL_THICKNESS};
    }
    wallState.count = WALL_SEGMENTS_PER_SIDE * WALL_SIDE_COUNT;
}

void wallsBeginRoom(float roomOffsetX, float roomOffsetY) {
    memset(&wallState, 0, sizeof(wallState));

    float segmentWidth = ROOM_WIDTH / (float)WALL_SEGMENTS_PER_SIDE;
    float segmentHeight = ROOM_HEIGHT / (float)WALL_SEGMENTS_PER_SIDE;

    for (int segment = 0; segment < WALL_SEGMENTS_PER_SIDE; segment++) {
        wallState.entries[segment] =
            (Wall){roomOffsetX - WALL_THICKNESS,
                   roomOffsetY + segment * segmentHeight,
                   WALL_THICKNESS * 2,
                   segmentHeight};
        wallState.entries[WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){roomOffsetX + ROOM_WIDTH - WALL_THICKNESS,
                   roomOffsetY + segment * segmentHeight,
                   WALL_THICKNESS * 2,
                   segmentHeight};
        wallState.entries[2 * WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){roomOffsetX + segment * segmentWidth,
                   roomOffsetY,
                   segmentWidth,
                   WALL_THICKNESS};
        wallState.entries[3 * WALL_SEGMENTS_PER_SIDE + segment] =
            (Wall){roomOffsetX + segment * segmentWidth,
                   roomOffsetY + ROOM_HEIGHT - WALL_THICKNESS,
                   segmentWidth,
                   WALL_THICKNESS};
    }
    wallState.count = WALL_SEGMENTS_PER_SIDE * WALL_SIDE_COUNT;
}

bool wallsAdd(Wall wall) {
    if (wallState.count >= MAX_WALLS) {
        return false;
    }
    wallState.entries[wallState.count] = wall;
    wallState.count++;
    return true;
}

void wallsRemoveBoundarySegment(WallSide side, int segment) {
    if (side < 0 || side >= WALL_SIDE_COUNT || segment < 0 ||
        segment >= WALL_SEGMENTS_PER_SIDE) {
        return;
    }

    int wallIndex = side * WALL_SEGMENTS_PER_SIDE + segment;
    wallState.entries[wallIndex].width = 0.0f;
    wallState.entries[wallIndex].height = 0.0f;
}

int wallsGetCount(void) {
    return wallState.count;
}

const Wall *wallsGet(int index) {
    if (index < 0 || index >= wallState.count) {
        return NULL;
    }
    return &wallState.entries[index];
}

void wallsDraw(void) {
    for (int index = 0; index < wallState.count; index++) {
        const Wall *wall = &wallState.entries[index];
        if (!wallIsPresent(wall)) {
            continue;
        }

        float screenX = 0.0f;
        float screenY = 0.0f;
        worldToScreen(wall->x, wall->y, &screenX, &screenY);
        riv_draw_rect_fill(screenX + WALL_DRAW_INSET,
                           screenY + WALL_DRAW_INSET,
                           wall->width - 2 * WALL_DRAW_INSET,
                           wall->height - 2 * WALL_DRAW_INSET,
                           RIV_COLOR_DARKGREEN);
    }
}

bool wallCircleContact(const Wall *wall,
                       float centerX,
                       float centerY,
                       float radius,
                       WallContact *contact) {
    if (!wallIsPresent(wall) || radius < 0.0f) {
        return false;
    }

    float closestX = fmaxf(wall->x, fminf(centerX, wall->x + wall->width));
    float closestY = fmaxf(wall->y, fminf(centerY, wall->y + wall->height));
    float deltaX = centerX - closestX;
    float deltaY = centerY - closestY;
    float distanceSquared = deltaX * deltaX + deltaY * deltaY;
    if (distanceSquared >= radius * radius) {
        return false;
    }

    if (contact != NULL) {
        *contact = (WallContact){
            .closestX = closestX,
            .closestY = closestY,
            .deltaX = deltaX,
            .deltaY = deltaY,
            .distance = sqrtf(distanceSquared)
        };
    }
    return true;
}

bool wallsPositionIsClear(float x, float y, float radius, float clearance) {
    float excludedRadius = radius + clearance;
    for (int index = 0; index < wallState.count; index++) {
        if (wallCircleContact(&wallState.entries[index], x, y, excludedRadius, NULL)) {
            return false;
        }
    }
    return true;
}
