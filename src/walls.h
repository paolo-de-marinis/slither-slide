#ifndef WALLS_H
#define WALLS_H

#include <stdbool.h>

#define WALL_SEGMENTS_PER_SIDE 8
#define WALL_DRAW_INSET 1.0f
#define MAX_WALLS (WALL_SEGMENTS_PER_SIDE * 4 + 10)

typedef enum {
    WALL_SIDE_LEFT,
    WALL_SIDE_RIGHT,
    WALL_SIDE_TOP,
    WALL_SIDE_BOTTOM,
    WALL_SIDE_COUNT
} WallSide;

/* Axis-aligned rectangle in global world coordinates. */
typedef struct {
    float x;
    float y;
    float width;
    float height;
} Wall;

typedef struct {
    float closestX;
    float closestY;
    float deltaX;
    float deltaY;
    float distance;
} WallContact;

void wallsInitializeMenu(void);
void wallsBeginRoom(float roomOffsetX, float roomOffsetY);
bool wallsAdd(Wall wall);
void wallsRemoveBoundarySegment(WallSide side, int segment);
int wallsGetCount(void);
const Wall *wallsGet(int index);
void wallsDraw(void);

bool wallCircleContact(const Wall *wall,
                       float centerX,
                       float centerY,
                       float radius,
                       WallContact *contact);
bool wallsPositionIsClear(float x, float y, float radius, float clearance);

#endif
