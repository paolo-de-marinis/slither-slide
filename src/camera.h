#ifndef CAMERA_H
#define CAMERA_H

#include <stdbool.h>
#include "room_layout.h"

// Initialize the camera system
void cameraInitialize(void);

// Update camera position
void cameraUpdate(void);

// Coordinate conversion helpers
void worldToScreen(float worldX, float worldY, float *screenX, float *screenY);

#endif // CAMERA_H
