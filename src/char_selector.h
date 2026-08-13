#ifndef CHAR_SELECTOR_H
#define CHAR_SELECTOR_H

#include "riv.h"

typedef enum {
    SKIN_SNAKE,
    SKIN_CATERPILLAR,
    SKIN_COUNT
} SkinType;

void initializeSkinSelector(void);
void drawSkinSelectionMenu(void);
void updateSkinSelection(void);
SkinType getCurrentSkin(void);
bool isSkinSelected(void);

#endif
