#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game_data.h"
#include "game_structs.h"
#include "render.h"
#include "update.h"

GameState gameState;
GameScreen currentScreen = SCREEN_MENU;
bool isEnglishMode = false;
bool showFPS = true;

// Hàm xáo trộn bộ bài (Fisher-Yates shuffle)
static void ShuffleDeck(const Card **deck, int count) {
  for (int i = count - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    const Card *temp = deck[i];
    deck[i] = deck[j];
    deck[j] = temp;
  }
}

void InitGameplay() {
  gameState.gameStatus = 0;
  gameState.playerLP = 8000;
  gameState.enemyLP = 8000;
  gameState.isPlayerTurn = true;
  gameState.selectedCardIndexInHand = -1;
  gameState.selectedAttacker = NULL;
  gameState.hoveredCard = NULL;

  // Bắt đầu từ Giai đoạn chuẩn bị (Preparation Phase)
  gameState.currentPhase = PHASE_PREPARATION;
  gameState.totalTurnCount = 1;
  gameState.normalSummonsThisTurn = 0;

  gameState.isRollingDice = false;
  gameState.playerDiceValue = 1;
  gameState.enemyDiceValue = 1;
  gameState.diceTimer = 0.0f;

  for (int i = 0; i < 5; i++) {
    gameState.playerAtkRow[i].isEmpty = true;
    gameState.playerAtkRow[i].card = NULL;
    gameState.playerAtkRow[i].isDefending = false;
    gameState.playerAtkRow[i].hasAttacked = false;
    gameState.playerAtkRow[i].summonedThisTurn = false;
    gameState.playerAtkRow[i].positionChangedThisTurn = false;

    gameState.playerDefRow[i].isEmpty = true;
    gameState.playerDefRow[i].card = NULL;
    gameState.playerDefRow[i].isDefending = true;
    gameState.playerDefRow[i].hasAttacked = false;
    gameState.playerDefRow[i].summonedThisTurn = false;
    gameState.playerDefRow[i].positionChangedThisTurn = false;

    gameState.enemyAtkRow[i].isEmpty = true;
    gameState.enemyAtkRow[i].card = NULL;
    gameState.enemyAtkRow[i].isDefending = false;
    gameState.enemyAtkRow[i].hasAttacked = false;
    gameState.enemyAtkRow[i].summonedThisTurn = false;
    gameState.enemyAtkRow[i].positionChangedThisTurn = false;

    gameState.enemyDefRow[i].isEmpty = true;
    gameState.enemyDefRow[i].card = NULL;
    gameState.enemyDefRow[i].isDefending = true;
    gameState.enemyDefRow[i].hasAttacked = false;
    gameState.enemyDefRow[i].summonedThisTurn = false;
    gameState.enemyDefRow[i].positionChangedThisTurn = false;
  }

  InitBattlefieldLayout(&gameState, GetScreenWidth(), GetScreenHeight());

  srand(time(NULL));

  // === Tạo bộ bài 40 lá cho Player ===
  gameState.playerDeckCount = DECK_SIZE;
  for (int i = 0; i < DECK_SIZE; i++) {
    gameState.playerDeck[i] = &cardDb[rand() % TOTAL_CARDS];
  }
  ShuffleDeck(gameState.playerDeck, gameState.playerDeckCount);

  // === Tạo bộ bài 40 lá cho Enemy ===
  gameState.enemyDeckCount = DECK_SIZE;
  for (int i = 0; i < DECK_SIZE; i++) {
    gameState.enemyDeck[i] = &cardDb[rand() % TOTAL_CARDS];
  }
  ShuffleDeck(gameState.enemyDeck, gameState.enemyDeckCount);

  // === Rút 5 lá ban đầu cho Player (từ deck) ===
  gameState.playerHandCount = 0;
  for (int i = 0; i < 5 && gameState.playerDeckCount > 0; i++) {
    gameState.playerDeckCount--;
    gameState.playerHand[gameState.playerHandCount++] =
        gameState.playerDeck[gameState.playerDeckCount];
  }

  // === Rút 5 lá ban đầu cho Enemy (từ deck) ===
  gameState.enemyHandCount = 0;
  for (int i = 0; i < 5 && gameState.enemyDeckCount > 0; i++) {
    gameState.enemyDeckCount--;
    gameState.enemyHand[gameState.enemyHandCount++] =
        gameState.enemyDeck[gameState.enemyDeckCount];
  }

  // === Khởi tạo animation rút bài ===
  gameState.drawAnim.active = false;
  gameState.drawAnim.timer = 0.0f;
  gameState.drawAnim.duration = 0.45f;
  gameState.drawAnim.card = NULL;
  gameState.drawAnim.showFront = false;
  gameState.drawAnim.flipTimer = 0.0f;
}

int main(void) {
  int screenWidth = 1280;
  int screenHeight = 720;

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, "Game Tam Quoc - YGO Engine");
  SetTargetFPS(60);

  ChangeDirectory(GetApplicationDirectory());
  InitAudioDevice();
  LoadAllCardTextures();
  LoadAllMusic();
  LoadPlayerData(); // ADD THIS
  PlayMusicStream(bgmMenu);

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_F9))
      showFPS = !showFPS;
    if (IsKeyPressed(KEY_F11))
      ToggleFullscreen();
    if (IsKeyPressed(KEY_F10)) {
      if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(screenWidth, screenHeight);
        SetWindowPosition(
            GetMonitorWidth(GetCurrentMonitor()) / 2 - screenWidth / 2,
            GetMonitorHeight(GetCurrentMonitor()) / 2 - screenHeight / 2);
      } else {
        if (IsWindowFullscreen())
          ToggleFullscreen();
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(GetMonitorWidth(GetCurrentMonitor()),
                      GetMonitorHeight(GetCurrentMonitor()));
        SetWindowPosition(0, 0);
      }
    }

    if (IsWindowResized() && currentScreen != SCREEN_MENU) {
      InitBattlefieldLayout(&gameState, GetScreenWidth(), GetScreenHeight());
    }

    Vector2 mousePoint = GetMousePosition();
    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();
    Rectangle btnPlay = {currentWidth / 2 - 150, currentHeight / 2 - 150, 300, 60};
    Rectangle btnShop = {currentWidth / 2 - 150, currentHeight / 2 - 60, 300, 60};
    Rectangle btnArenaSelect = {currentWidth / 2 - 150, currentHeight / 2 + 30, 300, 60};
    Rectangle btnLang = {currentWidth / 2 - 150, currentHeight / 2 + 120, 300, 60};
    Rectangle btnSettings = {currentWidth / 2 - 150, currentHeight / 2 + 210, 300, 60};

    switch (currentScreen) {
    case SCREEN_MENU:
      UpdateMusicStream(bgmMenu);
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mousePoint, btnPlay)) {
          InitGameplay();
          currentScreen = SCREEN_AI;
          StopMusicStream(bgmMenu);
          PlayMusicStream(bgmBattle);
        }
        if (CheckCollisionPointRec(mousePoint, btnShop)) {
          currentScreen = SCREEN_SHOP;
        }
        if (CheckCollisionPointRec(mousePoint, btnArenaSelect)) {
          currentScreen = SCREEN_ARENA_SELECT;
        }
        if (CheckCollisionPointRec(mousePoint, btnSettings)) {
          currentScreen = SCREEN_SETTINGS;
        }
        if (CheckCollisionPointRec(mousePoint, btnLang)) {
          isEnglishMode = !isEnglishMode;
        }
      }
      break;

    case SCREEN_SHOP:
      UpdateMusicStream(bgmMenu);
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle btnBack = {25, currentHeight - 60, 200, 40};
        if (CheckCollisionPointRec(mousePoint, btnBack)) {
          currentScreen = SCREEN_MENU;
        }
        
        Rectangle btnGacha = {currentWidth / 2 - 100, currentHeight / 2, 200, 60};
        if (CheckCollisionPointRec(mousePoint, btnGacha) && playerCoins >= 100) {
            // Find locked arenas
            int lockedIndices[7];
            int lockedCount = 0;
            for(int i=0; i<7; i++) {
                if(!arenaUnlocked[i]) {
                    lockedIndices[lockedCount++] = i;
                }
            }
            if(lockedCount > 0) {
                playerCoins -= 100;
                int randIdx = rand() % lockedCount;
                arenaUnlocked[lockedIndices[randIdx]] = true;
                SavePlayerData();
            }
        }
      }
      if (IsKeyPressed(KEY_BACKSPACE)) {
        currentScreen = SCREEN_MENU;
      }
      break;

    case SCREEN_ARENA_SELECT:
      UpdateMusicStream(bgmMenu);
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Rectangle btnBack = {25, currentHeight - 60, 200, 40};
        if (CheckCollisionPointRec(mousePoint, btnBack)) {
          currentScreen = SCREEN_MENU;
        }
        
        // Check click on arena buttons
        int startX = currentWidth / 2 - (3 * 200 + 2 * 20) / 2;
        int startY = 150;
        for(int i=0; i<7; i++) {
            if(arenaUnlocked[i]) {
                int row = i / 3;
                int col = i % 3;
                Rectangle arenaBtn = {startX + col * 220, startY + row * 160, 200, 120};
                if (CheckCollisionPointRec(mousePoint, arenaBtn)) {
                    currentArenaIndex = i;
                    SavePlayerData();
                }
            }
        }
      }
      if (IsKeyPressed(KEY_BACKSPACE)) {
        currentScreen = SCREEN_MENU;
      }
      break;

    case SCREEN_SETTINGS:
      UpdateMusicStream(bgmMenu);
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(
                mousePoint, (Rectangle){25, currentHeight - 60, 200, 40})) {
          currentScreen = SCREEN_MENU;
        }
      }
      if (IsKeyPressed(KEY_BACKSPACE)) {
        currentScreen = SCREEN_MENU;
      }
      break;

    case SCREEN_AI:
    case SCREEN_PVP:
    case SCREEN_STORY:
      if (gameState.gameStatus == 0) {
        UpdateMusicStream(bgmBattle);
      } else if (gameState.gameStatus == 1) {
        UpdateMusicStream(bgmWin);
      } else if (gameState.gameStatus == 2) {
        UpdateMusicStream(bgmLose);
      }

      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
          CheckCollisionPointRec(mousePoint, gameState.btnExitRect)) {
        currentScreen = SCREEN_MENU;
        StopMusicStream(bgmBattle);
        StopMusicStream(bgmWin);
        StopMusicStream(bgmLose);
        PlayMusicStream(bgmMenu);
      } else {
        int prevStatus = gameState.gameStatus;
        UpdateGameplay(&gameState);
        if (prevStatus == 0 && gameState.gameStatus != 0) {
          StopMusicStream(bgmBattle);
          if (gameState.gameStatus == 1) {
            PlayMusicStream(bgmWin);
            playerCoins += 100;
            SavePlayerData();
          }
          else if (gameState.gameStatus == 2)
            PlayMusicStream(bgmLose);
        }
      }
      if (IsKeyPressed(KEY_BACKSPACE)) {
        currentScreen = SCREEN_MENU;
        StopMusicStream(bgmBattle);
        StopMusicStream(bgmWin);
        StopMusicStream(bgmLose);
        PlayMusicStream(bgmMenu);
      }
      break;
    }

    BeginDrawing();
    switch (currentScreen) {
    case SCREEN_MENU:
      if (menuTexture.id > 0) {
        float scaleX = (float)currentWidth / menuTexture.width;
        float scaleY = (float)currentHeight / menuTexture.height;
        float scale = scaleX > scaleY ? scaleX : scaleY;
        DrawTextureEx(menuTexture, (Vector2){0, 0}, 0.0f, scale, WHITE);
      } else
        ClearBackground(DARKBLUE);

      DrawText("TAM QUOC CARD GAME",
               currentWidth / 2 - MeasureText("TAM QUOC CARD GAME", 50) / 2,
               currentHeight / 2 - 250, 50, GOLD);

      DrawRectangleRec(btnPlay, CheckCollisionPointRec(mousePoint, btnPlay)
                                    ? DARKGRAY
                                    : GRAY);
      DrawRectangleLinesEx(btnPlay, 2, WHITE);
      DrawText(
          isEnglishMode ? "PLAY AI MODE" : "CHOI VOI AI",
          btnPlay.x + btnPlay.width / 2 -
              MeasureText(isEnglishMode ? "PLAY AI MODE" : "CHOI VOI AI", 24) /
                  2,
          btnPlay.y + 18, 24, WHITE);
          
      DrawRectangleRec(btnShop, CheckCollisionPointRec(mousePoint, btnShop)
                                    ? DARKGRAY
                                    : GRAY);
      DrawRectangleLinesEx(btnShop, 2, WHITE);
      DrawText(
          isEnglishMode ? "SHOP (GACHA)" : "SHOP QUAY SAN",
          btnShop.x + btnShop.width / 2 -
              MeasureText(isEnglishMode ? "SHOP (GACHA)" : "SHOP QUAY SAN", 24) /
                  2,
          btnShop.y + 18, 24, WHITE);
          
      DrawRectangleRec(btnArenaSelect, CheckCollisionPointRec(mousePoint, btnArenaSelect)
                                    ? DARKGRAY
                                    : GRAY);
      DrawRectangleLinesEx(btnArenaSelect, 2, WHITE);
      DrawText(
          isEnglishMode ? "SELECT ARENA" : "CHON SAN DAU",
          btnArenaSelect.x + btnArenaSelect.width / 2 -
              MeasureText(isEnglishMode ? "SELECT ARENA" : "CHON SAN DAU", 24) /
                  2,
          btnArenaSelect.y + 18, 24, WHITE);

      DrawRectangleRec(btnLang, CheckCollisionPointRec(mousePoint, btnLang)
                                    ? DARKGRAY
                                    : GRAY);
      DrawRectangleLinesEx(btnLang, 2, WHITE);
      DrawText(isEnglishMode ? "LANGUAGE: ENGLISH" : "NGON NGU: TIENG VIET",
               btnLang.x + btnLang.width / 2 -
                   MeasureText(isEnglishMode ? "LANGUAGE: ENGLISH"
                                             : "NGON NGU: TIENG VIET",
                               24) /
                       2,
               btnLang.y + 18, 24, WHITE);

      DrawRectangleRec(
          btnSettings,
          CheckCollisionPointRec(mousePoint, btnSettings) ? DARKGRAY : GRAY);
      DrawRectangleLinesEx(btnSettings, 2, WHITE);
      DrawText(isEnglishMode ? "SETTINGS" : "CAI DAT",
               btnSettings.x + btnSettings.width / 2 -
                   MeasureText(isEnglishMode ? "SETTINGS" : "CAI DAT", 24) / 2,
               btnSettings.y + 18, 24, WHITE);
      break;

    case SCREEN_SHOP:
      ClearBackground(DARKBLUE);
      DrawText(isEnglishMode ? "ARENA SHOP" : "SHOP SAN DAU", currentWidth / 2 - MeasureText(isEnglishMode ? "ARENA SHOP" : "SHOP SAN DAU", 40) / 2, 50, 40, GOLD);
      
      char coinStr[64];
      sprintf(coinStr, isEnglishMode ? "Coins: %d" : "Xu: %d", playerCoins);
      DrawText(coinStr, currentWidth / 2 - MeasureText(coinStr, 30) / 2, 120, 30, YELLOW);
      
      int lockedCount = 0;
      for(int i=0; i<7; i++) {
          if(!arenaUnlocked[i]) lockedCount++;
      }
      
      Rectangle btnGacha = {currentWidth / 2 - 100, currentHeight / 2, 200, 60};
      if(lockedCount > 0) {
          DrawRectangleRec(btnGacha, CheckCollisionPointRec(mousePoint, btnGacha) ? DARKGRAY : GRAY);
          DrawRectangleLinesEx(btnGacha, 2, WHITE);
          DrawText(isEnglishMode ? "SPIN (100 Coins)" : "QUAY (100 Xu)", btnGacha.x + btnGacha.width/2 - MeasureText(isEnglishMode ? "SPIN (100 Coins)" : "QUAY (100 Xu)", 20)/2, btnGacha.y + 20, 20, WHITE);
      } else {
          DrawText(isEnglishMode ? "All Arenas Unlocked!" : "Da so huu tat ca san dau!", currentWidth / 2 - MeasureText(isEnglishMode ? "All Arenas Unlocked!" : "Da so huu tat ca san dau!", 30) / 2, currentHeight / 2, 30, GREEN);
      }
      
      Rectangle btnBackShop = {25, currentHeight - 60, 200, 40};
      DrawRectangleRec(btnBackShop, CheckCollisionPointRec(mousePoint, btnBackShop) ? MAROON : RED);
      DrawText(isEnglishMode ? "BACK" : "QUAY LAI", btnBackShop.x + btnBackShop.width / 2 - MeasureText(isEnglishMode ? "BACK" : "QUAY LAI", 20) / 2, btnBackShop.y + 10, 20, WHITE);
      break;

    case SCREEN_ARENA_SELECT:
      ClearBackground(DARKBLUE);
      DrawText(isEnglishMode ? "SELECT ARENA" : "CHON SAN DAU", currentWidth / 2 - MeasureText(isEnglishMode ? "SELECT ARENA" : "CHON SAN DAU", 40) / 2, 50, 40, GOLD);
      
      int startX = currentWidth / 2 - (3 * 200 + 2 * 20) / 2;
      int startY = 150;
      for(int i=0; i<7; i++) {
          if(arenaUnlocked[i]) {
              int row = i / 3;
              int col = i % 3;
              Rectangle arenaBtn = {startX + col * 220, startY + row * 160, 200, 120};
              
              if(arenaTextures[i].id > 0) {
                  DrawTexturePro(arenaTextures[i], (Rectangle){0,0,arenaTextures[i].width,arenaTextures[i].height}, arenaBtn, (Vector2){0,0}, 0.0f, WHITE);
              } else {
                  DrawRectangleRec(arenaBtn, GRAY);
              }
              
              if(currentArenaIndex == i) {
                  DrawRectangleLinesEx(arenaBtn, 4, RED);
                  DrawText(isEnglishMode ? "SELECTED" : "DANG CHON", arenaBtn.x + 10, arenaBtn.y + 10, 20, RED);
              } else {
                  DrawRectangleLinesEx(arenaBtn, 2, WHITE);
              }
              
              if(CheckCollisionPointRec(mousePoint, arenaBtn)) {
                  DrawRectangleLinesEx(arenaBtn, 4, GOLD);
              }
          }
      }
      
      Rectangle btnBackArena = {25, currentHeight - 60, 200, 40};
      DrawRectangleRec(btnBackArena, CheckCollisionPointRec(mousePoint, btnBackArena) ? MAROON : RED);
      DrawText(isEnglishMode ? "BACK" : "QUAY LAI", btnBackArena.x + btnBackArena.width / 2 - MeasureText(isEnglishMode ? "BACK" : "QUAY LAI", 20) / 2, btnBackArena.y + 10, 20, WHITE);
      break;

    case SCREEN_SETTINGS:
      ClearBackground(DARKBLUE);
      DrawText(isEnglishMode ? "SETTINGS" : "CAI DAT",
               currentWidth / 2 -
                   MeasureText(isEnglishMode ? "SETTINGS" : "CAI DAT", 40) / 2,
               50, 40, GOLD);

      DrawText(isEnglishMode ? "F9: Toggle FPS" : "F9: Bat/Tat FPS",
               currentWidth / 2 - 150, currentHeight / 2 - 40, 24, WHITE);
      DrawText(isEnglishMode ? "F10: Toggle Borderless"
                             : "F10: Che do khong vien",
               currentWidth / 2 - 150, currentHeight / 2, 24, WHITE);
      DrawText(isEnglishMode ? "F11: Toggle Fullscreen" : "F11: Toan man hinh",
               currentWidth / 2 - 150, currentHeight / 2 + 40, 24, WHITE);

      Rectangle btnBack = {25, currentHeight - 60, 200, 40};
      DrawRectangleRec(
          btnBack, CheckCollisionPointRec(mousePoint, btnBack) ? MAROON : RED);
      DrawText(isEnglishMode ? "BACK" : "QUAY LAI",
               btnBack.x + btnBack.width / 2 -
                   MeasureText(isEnglishMode ? "BACK" : "QUAY LAI", 20) / 2,
               btnBack.y + 10, 20, WHITE);
      break;

    case SCREEN_AI:
    case SCREEN_PVP:
    case SCREEN_STORY:
      DrawBattlefield(&gameState);
      if (gameState.gameStatus == 1) {
        DrawText(
            isEnglishMode ? "YOU WIN!" : "BAN DA THANG!",
            currentWidth / 2 -
                MeasureText(isEnglishMode ? "YOU WIN!" : "BAN DA THANG!", 50) /
                    2,
            currentHeight / 2 - 50, 50, GREEN);
      } else if (gameState.gameStatus == 2) {
        DrawText(
            isEnglishMode ? "YOU LOSE!" : "BAN DA THUA!",
            currentWidth / 2 -
                MeasureText(isEnglishMode ? "YOU LOSE!" : "BAN DA THUA!", 50) /
                    2,
            currentHeight / 2 - 50, 50, RED);
      }
      break;
    }

    if (showFPS) {
      DrawFPS(10, 10);
      DrawText("F9: Tat/Bat FPS | F10: Borderless | F11: Fullscreen", 10, 35,
               16, GREEN);
    }
    EndDrawing();
  }

  UnloadAllCardTextures();
  UnloadAllMusic();
  CloseAudioDevice();
  CloseWindow();
  return 0;
}