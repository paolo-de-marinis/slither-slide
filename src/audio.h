#ifndef AUDIO_H
#define AUDIO_H

#include "riv.h"

// Initialize audio system
void audioInitialize(void);

// Sound effects
void playStartSound(void);
void playEatSound(void);
void playEndSound(void);
void playDoorSound(void);
void playAppleBounceSound(void);
void playCoinBounceSound(void);
void playVictoryFanfare(void);

// Background music
void startBackgroundMusic(void);
void stopBackgroundMusic(void);
void playBackgroundMusic(void);

#endif // AUDIO_H