#include "developer_controls.h"

#include <assert.h>
#include <stdio.h>

int main(void) {
    DeveloperControls controls = {
        .technicalViewEnabled = true, .manualSpanSelection = true, .selectedSpan = 9};
    developerControlsInitialize(&controls);
    assert(!controls.technicalViewEnabled);
    assert(!controls.manualSpanSelection);
    assert(controls.selectedSpan == 0);

    developerControlsUpdate(&controls, false);
    assert(!controls.technicalViewEnabled);
    developerControlsUpdate(&controls, true);
    assert(controls.technicalViewEnabled);
    developerControlsUpdate(&controls, false);
    assert(controls.technicalViewEnabled);

    assert(developerControlsActiveSpan(&controls, 120, 10) == 2);
    assert(developerControlsSpanParameter(30) == 0.5f);

    developerControlsNavigateSpan(&controls, false, true, 120, 10);
    assert(controls.manualSpanSelection);
    assert(developerControlsActiveSpan(&controls, 120, 10) == 3);
    developerControlsNavigateSpan(&controls, false, true, 121, 10);
    assert(developerControlsActiveSpan(&controls, 121, 10) == 4);
    developerControlsNavigateSpan(&controls, true, false, 122, 10);
    assert(developerControlsActiveSpan(&controls, 122, 10) == 3);

    controls.selectedSpan = 0;
    developerControlsNavigateSpan(&controls, true, false, 123, 10);
    assert(developerControlsActiveSpan(&controls, 123, 10) == 9);
    developerControlsNavigateSpan(&controls, false, true, 124, 10);
    assert(developerControlsActiveSpan(&controls, 124, 10) == 0);

    controls.selectedSpan = 7;
    assert(developerControlsActiveSpan(&controls, 125, 10) == 7);
    developerControlsNavigateSpan(&controls, false, false, 125, 6);
    assert(controls.selectedSpan == 1);
    assert(developerControlsActiveSpan(&controls, 125, 12) == 1);

    developerControlsUpdate(&controls, true);
    assert(!controls.technicalViewEnabled);
    developerControlsNavigateSpan(&controls, false, true, 126, 12);
    assert(developerControlsActiveSpan(&controls, 126, 12) == 1);

    puts("developer controls: ok");
    return 0;
}
