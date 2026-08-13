#ifndef CAMERA_H
#define CAMERA_H

#include <stdbool.h>
#include "room_layout.h"

void cameraInitialize(void);

void cameraUpdate(void);

void worldToScreen(float worldX, float worldY, float *screenX, float *screenY);

#endif
