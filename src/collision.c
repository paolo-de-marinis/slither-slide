#include "collision.h"

#include "levels.h"
#include "snake_geometry.h"
#include "walls.h"

/* Preserve the published gameplay geometry independently of the selected skin. */
static const float HEAD_COLLISION_RADIUS = 8.0f * 0.67f;

static bool collidesWithRoomEdge(float headX,
                                 float headY,
                                 float headRadius,
                                 int roomX,
                                 int roomY);
static bool collidesWithWall(riv_vec2i nextHeadPosition,
                             float headX,
                             float headY,
                             float headRadius);
static bool collidesWithBody(const GameData *state,
                             float headX,
                             float headY,
                             float headRadius);

float collisionHeadRadius(void) {
    return HEAD_COLLISION_RADIUS;
}

bool collisionAtNextHead(const GameData *state, riv_vec2i nextHeadPosition) {
    if (nextHeadPosition.x < 0 || nextHeadPosition.x >= WORLD_TILES_X ||
        nextHeadPosition.y < 0 || nextHeadPosition.y >= WORLD_TILES_Y) {
        return true;
    }

    int roomX = 0;
    int roomY = 0;
    if (!getRoomPosition(getCurrentLevel(), &roomX, &roomY)) {
        return true;
    }

    float headX = nextHeadPosition.x * TILE_SIZE + TILE_SIZE / 2.0f;
    float headY = nextHeadPosition.y * TILE_SIZE + TILE_SIZE / 2.0f;
    float headRadius = collisionHeadRadius();
    return collidesWithRoomEdge(headX, headY, headRadius, roomX, roomY) ||
           collidesWithWall(nextHeadPosition, headX, headY, headRadius) ||
           collidesWithBody(state, headX, headY, headRadius);
}

static bool collidesWithRoomEdge(float headX,
                                 float headY,
                                 float headRadius,
                                 int roomX,
                                 int roomY) {
    float roomLeft = roomX * ROOM_WIDTH;
    float roomRight = (roomX + 1) * ROOM_WIDTH;
    float roomTop = roomY * ROOM_HEIGHT;
    float roomBottom = (roomY + 1) * ROOM_HEIGHT;
    float horizontalDoorStart =
        roomTop + DOOR_START_SEGMENT * ROOM_HEIGHT / (float)WALL_SEGMENTS_PER_SIDE;
    float horizontalDoorEnd = roomTop +
                              (DOOR_START_SEGMENT + DOOR_SEGMENT_COUNT) * ROOM_HEIGHT /
                                  (float)WALL_SEGMENTS_PER_SIDE;
    float verticalDoorStart =
        roomLeft + DOOR_START_SEGMENT * ROOM_WIDTH / (float)WALL_SEGMENTS_PER_SIDE;
    float verticalDoorEnd = roomLeft +
                            (DOOR_START_SEGMENT + DOOR_SEGMENT_COUNT) * ROOM_WIDTH /
                                (float)WALL_SEGMENTS_PER_SIDE;
    bool inHorizontalDoor =
        headY >= horizontalDoorStart - headRadius && headY <= horizontalDoorEnd + headRadius;
    bool inVerticalDoor =
        headX >= verticalDoorStart - headRadius && headX <= verticalDoorEnd + headRadius;

    if (headX < roomLeft + WALL_THICKNESS) {
        return !(inHorizontalDoor && canTransitionToLevel(getLevelAtPosition(roomX - 1, roomY)));
    }
    if (headX > roomRight - WALL_THICKNESS) {
        return !(inHorizontalDoor && canTransitionToLevel(getLevelAtPosition(roomX + 1, roomY)));
    }
    if (headY < roomTop + WALL_THICKNESS) {
        return !(inVerticalDoor && canTransitionToLevel(getLevelAtPosition(roomX, roomY - 1)));
    }
    if (headY > roomBottom - WALL_THICKNESS) {
        return !(inVerticalDoor && canTransitionToLevel(getLevelAtPosition(roomX, roomY + 1)));
    }
    return false;
}

static bool collidesWithWall(riv_vec2i nextHeadPosition,
                             float headX,
                             float headY,
                             float headRadius) {
    for (int index = 0; index < wallsGetCount(); index++) {
        const Wall *wall = wallsGet(index);
        Wall drawnObstacle;
        if (index >= WALL_SEGMENTS_PER_SIDE * WALL_SIDE_COUNT && wall != NULL) {
            drawnObstacle = (Wall){wall->x + WALL_DRAW_INSET,
                                   wall->y + WALL_DRAW_INSET,
                                   wall->width - 2 * WALL_DRAW_INSET,
                                   wall->height - 2 * WALL_DRAW_INSET};
            wall = &drawnObstacle;
        }
        if (!wallCircleContact(wall, headX, headY, headRadius, NULL)) {
            continue;
        }
        if (DEBUG_MODE) {
            riv_printf("Game Over: wall collision at (%d, %d)\n",
                       nextHeadPosition.x,
                       nextHeadPosition.y);
        }
        return true;
    }
    return false;
}

static bool collidesWithBody(const GameData *state,
                             float headX,
                             float headY,
                             float headRadius) {
    const int segmentSamples = 4;
    for (int index = 4; index < state->jointCount - 1; index++) {
        float collisionRadius = snakeBodyWidth(index) + headRadius;
        for (int sample = 0; sample <= segmentSamples; sample++) {
            float t = sample / (float)segmentSamples;
            float segmentX = state->joints[index - 1].x * (1.0f - t) +
                             state->joints[index].x * t;
            float segmentY = state->joints[index - 1].y * (1.0f - t) +
                             state->joints[index].y * t;
            float deltaX = headX - segmentX;
            float deltaY = headY - segmentY;
            if (deltaX * deltaX + deltaY * deltaY < collisionRadius * collisionRadius) {
                if (DEBUG_MODE) {
                    riv_printf("Game Over: self collision with segment %d\n", index);
                }
                return true;
            }
        }
    }
    return false;
}
