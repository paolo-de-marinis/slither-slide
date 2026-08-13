#include "developer_controls.h"

enum { ACTIVE_SPAN_FRAMES = 60 };

static int wrapSpan(int span, int controlPointCount) {
    if (controlPointCount <= 0) {
        return 0;
    }
    int wrapped = span % controlPointCount;
    return wrapped < 0 ? wrapped + controlPointCount : wrapped;
}

void developerControlsInitialize(DeveloperControls *controls) {
    controls->technicalViewEnabled = false;
    controls->manualSpanSelection = false;
    controls->selectedSpan = 0;
}

void developerControlsUpdate(DeveloperControls *controls, bool technicalViewPressed) {
    if (technicalViewPressed) {
        controls->technicalViewEnabled = !controls->technicalViewEnabled;
    }
}

int developerControlsActiveSpan(const DeveloperControls *controls,
                                uint64_t frame,
                                int controlPointCount) {
    if (controlPointCount <= 0) {
        return 0;
    }
    if (controls->manualSpanSelection) {
        return wrapSpan(controls->selectedSpan, controlPointCount);
    }
    return (int)((frame / ACTIVE_SPAN_FRAMES) % (uint64_t)controlPointCount);
}

float developerControlsSpanParameter(uint64_t frame) {
    return (frame % ACTIVE_SPAN_FRAMES) / (float)ACTIVE_SPAN_FRAMES;
}

void developerControlsNavigateSpan(DeveloperControls *controls,
                                   bool previousSpanPressed,
                                   bool nextSpanPressed,
                                   uint64_t frame,
                                   int controlPointCount) {
    if (controlPointCount <= 0) {
        return;
    }
    if (controls->manualSpanSelection) {
        controls->selectedSpan = wrapSpan(controls->selectedSpan, controlPointCount);
    }
    if (!controls->technicalViewEnabled || previousSpanPressed == nextSpanPressed) {
        return;
    }

    int currentSpan = developerControlsActiveSpan(controls, frame, controlPointCount);
    controls->selectedSpan =
        wrapSpan(currentSpan + (nextSpanPressed ? 1 : -1), controlPointCount);
    controls->manualSpanSelection = true;
}
