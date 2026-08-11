#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256
#define CENTER_X (SCREEN_WIDTH / 2)
#define CENTER_Y (SCREEN_HEIGHT / 2)

// Vertical positions (as percentages of screen height)
#define TOP_FIFTH (SCREEN_HEIGHT / 5)        // 20%
#define TOP_THIRD (SCREEN_HEIGHT / 3)        // 33%
#define MIDDLE (SCREEN_HEIGHT / 2)           // 50%
#define BOTTOM_THIRD (SCREEN_HEIGHT * 2 / 3) // 66%
#define BOTTOM_FIFTH (SCREEN_HEIGHT * 4 / 5) // 80%

// Preview position
#define PREVIEW_Y (SCREEN_HEIGHT * 5 / 8) // 62.5%, between MIDDLE and BOTTOM_THIRD

// Spacing values
#define STANDARD_SPACING (SCREEN_HEIGHT / 10) // 10% of screen height
#define SMALL_SPACING (SCREEN_HEIGHT / 20)    // 5% of screen height
#define LARGE_SPACING (SCREEN_HEIGHT / 8)     // 12.5% of screen height

// Text scaling
#define TITLE_SCALE 2.0f
#define NORMAL_SCALE 1.0f
#define SMALL_SCALE 0.8f

#endif // UI_CONSTANTS_H
