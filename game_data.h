#ifndef GAME_DATA_H
#define GAME_DATA_H

#include "game_structs.h"

// Tổng số lá bài trong database
#define TOTAL_CARDS 58

// Database bài
extern Card cardDb[TOTAL_CARDS];

// Textures toàn cục
extern Texture2D bgTexture;
extern Texture2D menuTexture;
extern Texture2D cardBackTexture;

// Hàm load/unload
void LoadAllCardTextures(void);
void UnloadAllCardTextures(void);

#endif // GAME_DATA_H