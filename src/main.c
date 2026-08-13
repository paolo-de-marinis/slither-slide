#include "audio.h"
#include "game.h"
#include "game_render.h"
#include "riv.h"
#include "ui_constants.h"

int main(void) {
    riv->width = SCREEN_WIDTH;
    riv->height = SCREEN_HEIGHT;
    riv->target_fps = TARGET_FPS;

    audioInitialize();
    gameInitialize();
    while (riv_present()) {
        gameUpdate();
        gameDraw();
        playBackgroundMusic();
    }
    return 0;
}
