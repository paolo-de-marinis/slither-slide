#ifndef AUDIO_H
#define AUDIO_H

#include "riv.h"

void audioInitialize(void);

void playStartSound(void);
void playEatSound(void);
void playEndSound(void);
void playDoorSound(void);
void playAppleBounceSound(void);
void playCoinBounceSound(void);
void playVictoryFanfare(void);

void startBackgroundMusic(void);
void stopBackgroundMusic(void);
void playBackgroundMusic(void);

#endif
