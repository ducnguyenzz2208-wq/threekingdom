#ifndef GAME_DATA_H
#define GAME_DATA_H

#include "game_structs.h"

// Tổng số lá bài trong database
#define TOTAL_CARDS 72

// Database bài
extern Card cardDb[TOTAL_CARDS];

// Textures toàn cục
extern Texture2D arenaTextures[7];
extern Texture2D menuTexture;
extern Texture2D cardBackTexture;

// Dữ liệu người chơi
extern bool arenaUnlocked[7];
extern int currentArenaIndex;
extern int playerCoins;

// Hàm load/unload
void LoadAllCardTextures(void);
void UnloadAllCardTextures(void);

// Hàm lưu/tải dữ liệu
void SavePlayerData(void);
void LoadPlayerData(void);

// Audio globals
extern Music bgmMenu;
extern Music bgmBattle;
extern Music bgmWin;
extern Music bgmLose;

void LoadAllMusic(void);
void UnloadAllMusic(void);

#endif // GAME_DATA_H