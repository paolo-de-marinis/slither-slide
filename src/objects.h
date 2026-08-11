#ifndef OBJECTS_H
#define OBJECTS_H

#include <stdbool.h>
#include "riv.h"
#include "camera.h"

typedef struct {
    float x;
    float y;
    float vx;
    float vy;
    bool active;
} PhysicsObject;

typedef struct {
    float x;
    float y;
    float width;
    float height;
} Wall;

#define WALL_SEGMENTS_PER_SIDE 8  // Number of segments per wall side
#define MAX_WALLS (WALL_SEGMENTS_PER_SIDE * 4 + 10)  // 4 sides + internal walls

typedef struct {
    riv_vec2i applePosition;
    PhysicsObject applePhysics;
    float friction;
    Wall walls[MAX_WALLS];
    int wallCount;
} ObjectState;

extern ObjectState object;

void objectsInitialize(void);
void drawApple(float rotation);
void drawWalls(void);
void updateApplePhysics(void);
bool spawnApple(void);

#endif
