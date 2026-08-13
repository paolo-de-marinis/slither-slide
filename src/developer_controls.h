#ifndef DEVELOPER_CONTROLS_H
#define DEVELOPER_CONTROLS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool technicalViewEnabled;
    bool manualSpanSelection;
    int selectedSpan;
} DeveloperControls;

void developerControlsInitialize(DeveloperControls *controls);
void developerControlsUpdate(DeveloperControls *controls, bool technicalViewPressed);
void developerControlsNavigateSpan(DeveloperControls *controls,
                                   bool previousSpanPressed,
                                   bool nextSpanPressed,
                                   uint64_t frame,
                                   int controlPointCount);
int developerControlsActiveSpan(const DeveloperControls *controls,
                                uint64_t frame,
                                int controlPointCount);
float developerControlsSpanParameter(uint64_t frame);

#endif
