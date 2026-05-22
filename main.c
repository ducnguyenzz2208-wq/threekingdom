#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game_structs.h"
#include "game_data.h"
#include "render.h"
#include "update.h"

// Biến toàn cục quản lý State Game
GameState gameState;
GameScreen currentScreen = SCREEN_MENU;
bool isEnglishMode = false;
bool showFPS = true; // Bật sẵn chế độ hiển thị FPS

// Hàm khởi tạo trạng thái bắt đầu game (Chia bài, set LP...)
void InitGameplay() {
    gameState.playerLP = 8000;
    gameState.enemyLP = 8000;
    gameState.isPlayerTurn = true;
    gameState.selectedCardIndexInHand = -1;
    gameState.selectedAttacker = NULL;
    gameState.hoveredCard = NULL;
    gameState.currentPhase = PHASE_MAIN;

    // Khởi tạo lưới sân đấu dựa trên kích thước màn hình hiện tại
    InitBattlefieldLayout(&gameState, GetScreenWidth(), GetScreenHeight());

    // Khởi tạo deck người chơi
    gameState.playerHandCount = 0;
    
    // Chia 5 lá ngẫu nhiên lên tay
    srand(time(NULL));
    for (int i = 0; i < 5; i++) {
        int r = rand() % TOTAL_CARDS;
        gameState.playerHand[gameState.playerHandCount++] = &cardDb[r];
    }
}

int main(void) {
    int screenWidth = 1280;
    int screenHeight = 720;

    // Cho phép resize cửa sổ để Windowed / Borderless hoạt động mượt mà
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Game Tam Quoc - C/Raylib Native");
    SetTargetFPS(60); // Khóa FPS ở mức 60
    
    // Đảm bảo game tìm đúng thư mục ảnh khi chạy
    ChangeDirectory(GetApplicationDirectory());
    LoadAllCardTextures();

    // Loop chính
    while (!WindowShouldClose()) {
        
        // --- XỬ LÝ PHÍM TẮT HỆ THỐNG ---
        if (IsKeyPressed(KEY_F9)) showFPS = !showFPS; // F9: Bật/tắt FPS
        
        if (IsKeyPressed(KEY_F11)) {
            // F11: Chế độ Fullscreen chuẩn (Độc quyền màn hình)
            ToggleFullscreen();
        }
        
        if (IsKeyPressed(KEY_F10)) {
            // F10: Chế độ Borderless Windowed (Toàn màn hình không viền)
            if (IsWindowState(FLAG_WINDOW_UNDECORATED)) {
                // Đang ở Borderless -> Trở về Windowed (Cửa sổ thường)
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(screenWidth, screenHeight);
                SetWindowPosition(GetMonitorWidth(GetCurrentMonitor())/2 - screenWidth/2, GetMonitorHeight(GetCurrentMonitor())/2 - screenHeight/2);
            } else {
                // Đang ở Windowed -> Lên Borderless
                if (IsWindowFullscreen()) ToggleFullscreen(); // Nếu đang Fullscreen thì thoát ra trước
                SetWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
                SetWindowPosition(0, 0);
            }
        }

        // Cập nhật lại layout sân đấu tự động nếu người chơi thay đổi kích thước cửa sổ
        if (IsWindowResized() && currentScreen != SCREEN_MENU) {
            InitBattlefieldLayout(&gameState, GetScreenWidth(), GetScreenHeight());
        }

        Vector2 mousePoint = GetMousePosition();
        
        // Cập nhật tọa độ nút Menu theo màn hình thực tế (khi bị scale / resize)
        int currentWidth = GetScreenWidth();
        int currentHeight = GetScreenHeight();
        Rectangle btnPlay = { currentWidth/2 - 150, currentHeight/2 - 60, 300, 60 };
        Rectangle btnLang = { currentWidth/2 - 150, currentHeight/2 + 30, 300, 60 };
        
        // ---- UPDATE LOGIC ----
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
                UpdateGameplay(&gameState);
                if (IsKeyPressed(KEY_BACKSPACE)) {
                    currentScreen = SCREEN_MENU;
                }
                break;
        }
        
        // ---- DRAW LOGIC ----
        BeginDrawing();
            
        switch (currentScreen) {
            case SCREEN_MENU:
                if (menuTexture.id > 0) {
                    float scaleX = (float)currentWidth / menuTexture.width;
                    float scaleY = (float)currentHeight / menuTexture.height;
                    float scale = scaleX > scaleY ? scaleX : scaleY; // Giữ tỷ lệ cover toàn màn hình
                    DrawTextureEx(menuTexture, (Vector2){0, 0}, 0.0f, scale, WHITE);
                } else {
                    ClearBackground(DARKBLUE);
                }
                
                DrawText("TAM QUOC CARD GAME", currentWidth/2 - MeasureText("TAM QUOC CARD GAME", 50)/2, currentHeight/2 - 180, 50, GOLD);

                // Nút PLAY
                DrawRectangleRec(btnPlay, CheckCollisionPointRec(mousePoint, btnPlay) ? DARKGRAY : GRAY);
                DrawRectangleLinesEx(btnPlay, 2, WHITE);
                const char* playText = isEnglishMode ? "PLAY AI MODE" : "CHOI VOI AI";
                DrawText(playText, btnPlay.x + btnPlay.width/2 - MeasureText(playText, 24)/2, btnPlay.y + 18, 24, WHITE);

                // Nút ĐỔI NGÔN NGỮ
                DrawRectangleRec(btnLang, CheckCollisionPointRec(mousePoint, btnLang) ? DARKGRAY : GRAY);
                DrawRectangleLinesEx(btnLang, 2, WHITE);
                const char* langText = isEnglishMode ? "LANGUAGE: ENGLISH" : "NGON NGU: TIENG VIET";
                DrawText(langText, btnLang.x + btnLang.width/2 - MeasureText(langText, 24)/2, btnLang.y + 18, 24, WHITE);
                
                break;
                
            case SCREEN_AI:
            case SCREEN_PVP:
            case SCREEN_STORY:
                DrawBattlefield(&gameState);
                break;
        }

        // Vẽ thông số FPS và phím tắt lên góc trái màn hình (chỉ vẽ khi showFPS = true)
        if (showFPS) {
            DrawFPS(10, 10);
            DrawText("F9: Tat/Bat FPS | F10: Borderless | F11: Fullscreen", 10, 35, 16, GREEN);
        }
        
        EndDrawing();
    }

    // Dọn dẹp
    UnloadAllCardTextures();
    CloseWindow();

    return 0;
}