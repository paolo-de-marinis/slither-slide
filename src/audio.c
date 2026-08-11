#define SEQT_IMPL
#include "seqt.h"
#include "audio.h"
#include "riv.h"
#include <stdio.h>
#include "game.h"

// Background music
static seqt_source *backgroundMusic = NULL;
static uint64_t backgroundMusicId = 0;
static bool playingMusic = false;

// Victory fanfare sound effects
static riv_waveform_desc fanfareSounds[] = {{
                                                .type = RIV_WAVEFORM_TRIANGLE,
                                                .attack = 0.05f,
                                                .decay = 0.1f,
                                                .sustain = 0.2f,
                                                .release = 0.1f,
                                                .start_frequency = RIV_NOTE_C5,
                                                .end_frequency = RIV_NOTE_C5,
                                                .amplitude = 0.25f,
                                                .sustain_level = 0.5f,
                                            },
                                            {
                                                .type = RIV_WAVEFORM_TRIANGLE,
                                                .attack = 0.05f,
                                                .decay = 0.1f,
                                                .sustain = 0.2f,
                                                .release = 0.1f,
                                                .start_frequency = RIV_NOTE_E5,
                                                .end_frequency = RIV_NOTE_E5,
                                                .amplitude = 0.25f,
                                                .sustain_level = 0.5f,
                                            },
                                            {
                                                .type = RIV_WAVEFORM_TRIANGLE,
                                                .attack = 0.05f,
                                                .decay = 0.1f,
                                                .sustain = 0.3f,
                                                .release = 0.2f,
                                                .start_frequency = RIV_NOTE_G5,
                                                .end_frequency = RIV_NOTE_G5,
                                                .amplitude = 0.25f,
                                                .sustain_level = 0.5f,
                                            },
                                            {
                                                .type = RIV_WAVEFORM_TRIANGLE,
                                                .attack = 0.05f,
                                                .decay = 0.1f,
                                                .sustain = 0.4f,
                                                .release = 0.3f,
                                                .start_frequency = RIV_NOTE_C6,
                                                .end_frequency = RIV_NOTE_C6,
                                                .amplitude = 0.25f,
                                                .sustain_level = 0.5f,
                                            }};

// Sound effect definitions
static riv_waveform_desc startSound = {.type = RIV_WAVEFORM_SQUARE,
                                       .attack = 0.01f,
                                       .decay = 0.1f,
                                       .sustain = 0.1f,
                                       .release = 0.1f,
                                       .start_frequency = 440.0f,
                                       .end_frequency = 880.0f,
                                       .amplitude = 0.3f,
                                       .sustain_level = 0.5f};

static riv_waveform_desc eatSound = {.type = RIV_WAVEFORM_TRIANGLE,
                                     .attack = 0.01f,
                                     .decay = 0.1f,
                                     .sustain = 0.0f,
                                     .release = 0.1f,
                                     .start_frequency = 880.0f,
                                     .end_frequency = 440.0f,
                                     .amplitude = 0.3f,
                                     .sustain_level = 0.5f};

static riv_waveform_desc endSound = {.type = RIV_WAVEFORM_SAWTOOTH,
                                     .attack = 0.01f,
                                     .decay = 0.3f,
                                     .sustain = 0.2f,
                                     .release = 0.3f,
                                     .start_frequency = 440.0f,
                                     .end_frequency = 220.0f,
                                     .amplitude = 0.3f,
                                     .sustain_level = 0.5f};

static riv_waveform_desc doorSound = {.type = RIV_WAVEFORM_NOISE,
                                      .attack = 0.01f,
                                      .decay = 0.3f,
                                      .sustain = 0.1f,
                                      .release = 0.2f,
                                      .start_frequency = 200.0f,
                                      .end_frequency = 100.0f,
                                      .amplitude = 0.3f,
                                      .sustain_level = 0.5f};

static riv_waveform_desc appleBounceSound = {.type = RIV_WAVEFORM_TRIANGLE,
                                             .attack = 0.01f,
                                             .decay = 0.1f,
                                             .sustain = 0.0f,
                                             .release = 0.1f,
                                             .start_frequency = 440.0f,
                                             .end_frequency = 880.0f,
                                             .amplitude = 0.2f,
                                             .sustain_level = 0.5f};

static riv_waveform_desc coinBounceSound = {.type = RIV_WAVEFORM_SQUARE,
                                            .attack = 0.01f,
                                            .decay = 0.15f,
                                            .sustain = 0.0f,
                                            .release = 0.1f,
                                            .start_frequency = 880.0f,
                                            .end_frequency = 1760.0f,
                                            .amplitude = 0.2f,
                                            .sustain_level = 0.5f};

void audioInitialize(void) {
    seqt_init();
    backgroundMusic = seqt_make_source_from_file("songs/gameplay.rivcard");
    if (!backgroundMusic) {
        riv_printf("Failed to load background music\n");
        return;
    }
}

void playStartSound(void) {
    riv_waveform(&startSound);
}

void playEatSound(void) {
    riv_waveform(&eatSound);
}

void playEndSound(void) {
    riv_waveform(&endSound);
}

void playDoorSound(void) {
    riv_waveform(&doorSound);
}

void playAppleBounceSound(void) {
    riv_waveform(&appleBounceSound);
}

void playCoinBounceSound(void) {
    riv_waveform(&coinBounceSound);
}

void playVictoryFanfare(void) {
    for (size_t i = 0; i < sizeof(fanfareSounds) / sizeof(fanfareSounds[0]); i++) {
        riv_waveform(&fanfareSounds[i]);
    }
}

void playBackgroundMusic(void) {
    if (!playingMusic || !backgroundMusic) {
        return;
    }
    seqt_poll();
}

void startBackgroundMusic(void) {
    if (!backgroundMusic) {
        return;
    }
    playingMusic = true;
    backgroundMusicId = seqt_play(backgroundMusic, -1);
    if (DEBUG_MODE) {
        riv_printf("Starting background music, id: %lu\n", backgroundMusicId);
    }
}

void stopBackgroundMusic(void) {
    playingMusic = false;
    if (backgroundMusicId != 0) {
        seqt_stop(backgroundMusicId);
        backgroundMusicId = 0;
    }
}
