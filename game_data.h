#ifndef GAME_DATA_H
#define GAME_DATA_H

#include "game_structs.h"

#define TOTAL_CARDS 58

extern Card cardDb[TOTAL_CARDS];
extern Texture2D bgTexture;
extern Texture2D menuTexture;
extern Texture2D cardBackTexture;

void LoadAllCardTextures();
void UnloadAllCardTextures();

#endif