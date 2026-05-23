#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game_structs.h"
#include "game_data.h"
#include "render.h"
#include "update.h"

GameState gameState;
GameScreen currentScreen = SCREEN_MENU;
bool isEnglishMode = false;
bool showFPS = true; 

void InitGameplay() {
    gameState.playerLP = 8000;
    gameState.enemyLP = 8000;
    gameState.isPlayerTurn = true;
    gameState.selectedCardIndexInHand = -1;
    gameState.selectedAttacker = NULL;
    gameState.hoveredCard = NULL;
    
    // Bắt đầu từ Draw Phase của Turn 1
    gameState.currentPhase = PHASE_DRAW;
    gameState.totalTurnCount = 1;
    gameState.hasNormalSummonedThisTurn = false;

    for (int i = 0; i < 5; i++) {
        gameState.playerAtkRow[i].isEmpty = true;
        gameState.playerAtkRow[i].card = NULL;
        gameState.playerAtkRow[i].isDefending = false;
        gameState.playerAtkRow[i].hasAttacked = false;
        gameState.playerAtkRow[i].summonedThisTurn = false;
        
        gameState.playerDefRow[i].isEmpty = true;
        gameState.playerDefRow[i].card = NULL;
        gameState.playerDefRow[i].isDefending = true;
        gameState.playerDefRow[i].hasAttacked = false;
        gameState.playerDefRow[i].summonedThisTurn = false;

        gameState.enemyAtkRow[i].isEmpty = true;
        gameState.enemyAtkRow[i].card = NULL;
        gameState.enemyAtkRow[i].isDefending = false;
        gameState.enemyAtkRow[i].hasAttacked = false;
        gameState.enemyAtkRow[i].summonedThisTurn = false;

        gameState.enemyDefRow[i].isEmpty = true;
        gameState.enemyDefRow[i].card = NULL;
        gameState.enemyDefRow[i].isDefending = true;
        gameState.enemyDefRow[i].hasAttacked = false;
        gameState.enemyDefRow[i].summonedThisTurn = false;
    }

    InitBattlefieldLayout(&gameState, GetScreenWidth(), GetScreenHeight());
    gameState.playerHandCount = 0;
    
    srand(time(NULL));
    // Rút 5 lá ban đầu
    for (int i = 0; i < 5; i++) {
        int r = rand() % TOTAL_CARDS;
        gameState.playerHand[gameState.playerHandCount++] = &cardDb[r];
    }
}

int main(void) {
    int screenWidth = 1280;
    int screenHeight = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Game Tam Quoc - YGO Engine");
    SetTargetFPS(60); 
    
    ChangeDirectory(GetApplicationDirectory());
    LoadAllCardTextures();

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F9)) showFPS = !showFPS; 
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
        if (IsKeyPressed(KEY_F10)) {
            if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(screenWidth, screenHeight);
                SetWindowPosition(GetMonitorWidth(GetCurrentMonitor())/2 - screenWidth/2, GetMonitorHeight(GetCurrentMonitor())/2 - screenHeight/2);
            } else {
                if (IsWindowFullscreen()) ToggleFullscreen();
                SetWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
                SetWindowPosition(0, 0);
            }
        }

        if (IsWindowResized() && currentScreen != SCREEN_MENU) {
            InitBattlefieldLayout(&gameState, GetScreenWidth(), GetScreenHeight());
        }

        Vector2 mousePoint = GetMousePosition();
        int currentWidth = GetScreenWidth();
        int currentHeight = GetScreenHeight();
        Rectangle btnPlay = { currentWidth/2 - 150, currentHeight/2 - 60, 300, 60 };
        Rectangle btnLang = { currentWidth/2 - 150, currentHeight/2 + 30, 300, 60 };
        
        switch (currentScreen) {
            case SCREEN_MENU:
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (CheckCollisionPointRec(mousePoint, btnPlay)) {
                        InitGameplay();
                        currentScreen = SCREEN_AI;
                    }
                    if (CheckCollisionPointRec(mousePoint, btnLang)) {
                        isEnglishMode = !isEnglishMode;
                    }
                }
                break;
                
            case SCREEN_AI:
            case SCREEN_PVP:
            case SCREEN_STORY:
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePoint, gameState.btnExitRect)) {
                    currentScreen = SCREEN_MENU;
                } else {
                    UpdateGameplay(&gameState);
                }
                if (IsKeyPressed(KEY_BACKSPACE)) currentScreen = SCREEN_MENU;
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
                } else ClearBackground(DARKBLUE);
                
                DrawText("TAM QUOC CARD GAME", currentWidth/2 - MeasureText("TAM QUOC CARD GAME", 50)/2, currentHeight/2 - 180, 50, GOLD);
                DrawRectangleRec(btnPlay, CheckCollisionPointRec(mousePoint, btnPlay) ? DARKGRAY : GRAY);
                DrawRectangleLinesEx(btnPlay, 2, WHITE);
                DrawText(isEnglishMode ? "PLAY AI MODE" : "CHOI VOI AI", btnPlay.x + btnPlay.width/2 - MeasureText(isEnglishMode ? "PLAY AI MODE" : "CHOI VOI AI", 24)/2, btnPlay.y + 18, 24, WHITE);
                DrawRectangleRec(btnLang, CheckCollisionPointRec(mousePoint, btnLang) ? DARKGRAY : GRAY);
                DrawRectangleLinesEx(btnLang, 2, WHITE);
                DrawText(isEnglishMode ? "LANGUAGE: ENGLISH" : "NGON NGU: TIENG VIET", btnLang.x + btnLang.width/2 - MeasureText(isEnglishMode ? "LANGUAGE: ENGLISH" : "NGON NGU: TIENG VIET", 24)/2, btnLang.y + 18, 24, WHITE);
                break;
                
            case SCREEN_AI:
            case SCREEN_PVP:
            case SCREEN_STORY:
                DrawBattlefield(&gameState);
                break;
        }

        if (showFPS) {
            DrawFPS(10, 10);
            DrawText("F9: Tat/Bat FPS | F10: Borderless | F11: Fullscreen", 10, 35, 16, GREEN);
        }
        EndDrawing();
    }

    UnloadAllCardTextures();
    CloseWindow();
    return 0;
}