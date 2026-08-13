#include "body_chain.h"

#include "snake_geometry.h"
#include "walls.h"

#include <math.h>
#include <stdlib.h>

#define NORMAL_EPSILON 0.0001f

static void pinHeadJoint(GameData *state);
static void followPreviousJoint(GameData *state, int jointIndex);
static void resolveJointOverlap(GameData *state, int jointIndex);
static void pushJointFromWalls(GameData *state, int jointIndex);
static void updateJointTangents(GameData *state);

void bodyChainInitialize(GameData *state) {
    state->jointCount = INITIAL_SNAKE_LENGTH;
    for (int index = 0; index < state->jointCount; index++) {
        state->joints[index].x = (float)(state->headPosition.x * TILE_SIZE) + TILE_SIZE / 2.0f;
        state->joints[index].y =
            (float)((state->headPosition.y + index) * TILE_SIZE) + TILE_SIZE / 2.0f;
        state->joints[index].angle = -PI / 2.0f;
    }
    updateJointTangents(state);
}

void bodyChainGrow(GameData *state) {
    if (state->jointCount <= 0 || state->jointCount >= MAX_JOINTS || state->growthRate <= 0) {
        return;
    }

    int previousCount = state->jointCount;
    int nextCount = previousCount + state->growthRate;
    if (nextCount > MAX_JOINTS) {
        nextCount = MAX_JOINTS;
    }
    for (int index = previousCount; index < nextCount; index++) {
        state->joints[index] = state->joints[previousCount - 1];
    }
    state->jointCount = nextCount;
}

void bodyChainUpdate(GameData *state) {
    if (state->jointCount <= 0) {
        return;
    }

    pinHeadJoint(state);
    for (int index = 1; index < state->jointCount; index++) {
        followPreviousJoint(state, index);
        resolveJointOverlap(state, index);
        pushJointFromWalls(state, index);
    }

    /* The head is a boundary condition, not a free solver variable. */
    pinHeadJoint(state);
    updateJointTangents(state);
}

static void pinHeadJoint(GameData *state) {
    state->joints[0].x = (float)(state->headPosition.x * TILE_SIZE) + TILE_SIZE / 2.0f;
    state->joints[0].y = (float)(state->headPosition.y * TILE_SIZE) + TILE_SIZE / 2.0f;
}

static void followPreviousJoint(GameData *state, int jointIndex) {
    float deltaX = state->joints[jointIndex - 1].x - state->joints[jointIndex].x;
    float deltaY = state->joints[jointIndex - 1].y - state->joints[jointIndex].y;
    float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
    if (distance <= NORMAL_EPSILON) {
        return;
    }

    float moveFactor;
    if (distance > TILE_SIZE) {
        moveFactor = (distance - TILE_SIZE) / distance;
    } else {
        moveFactor = (distance - TILE_SIZE * 0.9f) / distance * 0.5f;
    }
    state->joints[jointIndex].x += deltaX * moveFactor;
    state->joints[jointIndex].y += deltaY * moveFactor;
}

static void resolveJointOverlap(GameData *state, int jointIndex) {
    /* Direct pair comparison is acceptable for the cartridge's playable lengths. */
    for (int otherIndex = 1; otherIndex < state->jointCount; otherIndex++) {
        if (abs(jointIndex - otherIndex) <= 2) {
            continue;
        }

        float deltaX = state->joints[jointIndex].x - state->joints[otherIndex].x;
        float deltaY = state->joints[jointIndex].y - state->joints[otherIndex].y;
        float distance = sqrtf(deltaX * deltaX + deltaY * deltaY);
        float combinedWidth =
            snakeBodyWidth(jointIndex) + snakeBodyWidth(otherIndex);
        float minimumDistance = combinedWidth * 0.8f;
        if (distance >= minimumDistance || distance <= NORMAL_EPSILON) {
            continue;
        }

        float pushX = deltaX / distance;
        float pushY = deltaY / distance;
        float pushAmount = (minimumDistance - distance) * 0.5f;
        state->joints[jointIndex].x += pushX * pushAmount;
        state->joints[jointIndex].y += pushY * pushAmount;
        state->joints[otherIndex].x -= pushX * pushAmount;
        state->joints[otherIndex].y -= pushY * pushAmount;

        for (int offset = 1; offset < 3; offset++) {
            if (jointIndex + offset < state->jointCount) {
                float fade = 0.5f / (float)offset;
                state->joints[jointIndex + offset].x += pushX * pushAmount * fade;
                state->joints[jointIndex + offset].y += pushY * pushAmount * fade;
            }
            if (otherIndex + offset < state->jointCount) {
                float fade = 0.5f / (float)offset;
                state->joints[otherIndex + offset].x -= pushX * pushAmount * fade;
                state->joints[otherIndex + offset].y -= pushY * pushAmount * fade;
            }
        }
    }
}

static void pushJointFromWalls(GameData *state, int jointIndex) {
    float collisionWidth = snakeBodyWidth(jointIndex);
    for (int wallIndex = 0; wallIndex < wallsGetCount(); wallIndex++) {
        const Wall *wall = wallsGet(wallIndex);
        WallContact contact;
        if (!wallCircleContact(wall,
                               state->joints[jointIndex].x,
                               state->joints[jointIndex].y,
                               collisionWidth,
                               &contact)) {
            continue;
        }

        if (contact.distance <= NORMAL_EPSILON) {
            if (wall->width < wall->height) {
                float center = wall->x + wall->width / 2.0f;
                float normal = state->joints[jointIndex].x < center ? -1.0f : 1.0f;
                float edge = normal < 0.0f ? wall->x : wall->x + wall->width;
                state->joints[jointIndex].x = edge + normal * collisionWidth;
            } else {
                float center = wall->y + wall->height / 2.0f;
                float normal = state->joints[jointIndex].y < center ? -1.0f : 1.0f;
                float edge = normal < 0.0f ? wall->y : wall->y + wall->height;
                state->joints[jointIndex].y = edge + normal * collisionWidth;
            }
            continue;
        }

        float normalX = contact.deltaX / contact.distance;
        float normalY = contact.deltaY / contact.distance;
        float pushDistance = (collisionWidth - contact.distance) * 0.5f;
        state->joints[jointIndex].x += normalX * pushDistance;
        state->joints[jointIndex].y += normalY * pushDistance;

        if (jointIndex > 1) {
            state->joints[jointIndex - 1].x += normalX * pushDistance * 0.5f;
            state->joints[jointIndex - 1].y += normalY * pushDistance * 0.5f;
        }
        for (int offset = 1; offset < 4 && jointIndex + offset < state->jointCount; offset++) {
            float fade = (float)(4 - offset) * 0.5f;
            state->joints[jointIndex + offset].x += normalX * pushDistance * fade;
            state->joints[jointIndex + offset].y += normalY * pushDistance * fade;
        }
    }
}

static void updateJointTangents(GameData *state) {
    if (state->jointCount == 1) {
        state->joints[0].angle =
            atan2f((float)state->headDirection.y, (float)state->headDirection.x);
        return;
    }

    float headDeltaX = state->joints[0].x - state->joints[1].x;
    float headDeltaY = state->joints[0].y - state->joints[1].y;
    state->joints[0].angle = atan2f(headDeltaY, headDeltaX);
    for (int index = 1; index < state->jointCount; index++) {
        float deltaX = state->joints[index - 1].x - state->joints[index].x;
        float deltaY = state->joints[index - 1].y - state->joints[index].y;
        state->joints[index].angle = atan2f(deltaY, deltaX);
    }
}
