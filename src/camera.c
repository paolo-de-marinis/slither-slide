#include "camera.h"

#include "game.h"
#include "levels.h"

#include <math.h>

static struct {
    float x;
    float y;
    float targetX;
    float targetY;
    float smoothing;
    bool transitioning;
} camera = {
    .smoothing = 0.15f
};

extern GameData game;

void cameraInitialize(void) {
    camera.x = 0.0f;
    camera.y = 0.0f;
    camera.targetX = 0.0f;
    camera.targetY = 0.0f;
    camera.transitioning = false;
}

void cameraUpdate(void) {
    int roomX = 0;
    int roomY = 0;

    if (!game.started || !getRoomPosition(getCurrentLevel(), &roomX, &roomY)) {
        return;
    }

    float targetX = roomX * ROOM_WIDTH;
    float targetY = roomY * ROOM_HEIGHT;
    if (targetX != camera.targetX || targetY != camera.targetY) {
        camera.targetX = targetX;
        camera.targetY = targetY;
        camera.transitioning = true;
    }

    if (!camera.transitioning) {
        return;
    }

    float distanceX = camera.targetX - camera.x;
    float distanceY = camera.targetY - camera.y;
    camera.x += distanceX * camera.smoothing;
    camera.y += distanceY * camera.smoothing;

    if (fabsf(distanceX) < 0.5f && fabsf(distanceY) < 0.5f) {
        camera.x = camera.targetX;
        camera.y = camera.targetY;
        camera.transitioning = false;
    }
}

void worldToScreen(float worldX, float worldY, float *screenX, float *screenY) {
    *screenX = worldX - camera.x;
    *screenY = worldY - camera.y;
}
