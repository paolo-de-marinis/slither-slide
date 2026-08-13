#include "audio.h"
#include "char_selector.h"
#include "controls.h"
#include "developer_controls.h"
#include "game.h"
#include "game_render.h"
#include "riv.h"
#include "ui_constants.h"

int main(void) {
    DeveloperControls developerControls;
    riv->width = SCREEN_WIDTH;
    riv->height = SCREEN_HEIGHT;
    riv->target_fps = TARGET_FPS;
    riv->tracked_keys[CONTROL_TECHNICAL_VIEW] = true;
    riv->tracked_keys[CONTROL_TECHNICAL_PREVIOUS_SPAN] = true;
    riv->tracked_keys[CONTROL_TECHNICAL_NEXT_SPAN] = true;
#if CHEATS_ENABLED
    riv->tracked_keys[CONTROL_CHEAT_NEXT_LEVEL] = true;
#endif

    audioInitialize();
    gameInitialize();
    developerControlsInitialize(&developerControls);
    while (riv_present()) {
        developerControlsUpdate(
            &developerControls, riv->keys[CONTROL_TECHNICAL_VIEW].press);
        gameUpdate();
        gameDraw(&developerControls);
        if (game.state == GAME_STATE_PLAYING && getCurrentSkin() == SKIN_SNAKE) {
            developerControlsNavigateSpan(
                &developerControls,
                riv->keys[CONTROL_TECHNICAL_PREVIOUS_SPAN].press,
                riv->keys[CONTROL_TECHNICAL_NEXT_SPAN].press,
                riv->frame,
                snakeBodyGeometry()->controlPointCount);
        }
        playBackgroundMusic();
    }
    return 0;
}
