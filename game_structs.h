#ifndef GAME_STRUCTS_H
#define GAME_STRUCTS_H

#include "raylib.h"
#include <stdbool.h>

extern bool isEnglishMode;

#define MAX_HAND_CARDS 10
#define DECK_SIZE 40

typedef enum { TYPE_VO, TYPE_VAN } CardType;
typedef enum { KINGDOM_THUC, KINGDOM_NGUY, KINGDOM_NGO, KINGDOM_QUAN } Kingdom;

typedef struct {
    int id;
    char name_vn[50];    
    char name_en[50];    
    int stars;           // Tương đương Level trong YGO
    CardType type;
    Kingdom kingdom;
    int atk;
    int def;
    char imgPath[100];
    char desc_vn[255];   
    char desc_en[255];   
    Texture2D texture;   
} Card;

// Trạng thái hiệu ứng rút bài
typedef struct {
    bool active;           // Đang chạy animation?
    float timer;           // Thời gian hiện tại (0 -> duration)
    float duration;        // Tổng thời gian animation (giây)
    Vector2 startPos;      // Vị trí xuất phát (deck)
    Vector2 endPos;        // Vị trí đích (tay bài)
    float startW, startH;  // Kích thước lúc bắt đầu
    float endW, endH;      // Kích thước lúc kết thúc
    const Card* card;      // Lá bài đang rút
    bool showFront;        // Hiện mặt trước hay mặt sau
    float flipTimer;       // Timer lật bài
} DrawAnimState;

// 6 Phase chuẩn của Yu-Gi-Oh!
typedef enum {
    PHASE_DRAW = 0,
    PHASE_STANDBY,
    PHASE_MAIN_1,
    PHASE_BATTLE,
    PHASE_MAIN_2,
    PHASE_END
} GamePhase;

typedef struct {
    const Card* card; 
    bool isEmpty;
    Rectangle rect;   
    bool isDefending; 
    bool hasAttacked; 
    bool summonedThisTurn; // Cờ khóa đổi tư thế chiến đấu trong lượt đầu tiên
} Slot;

typedef struct {
    int playerLP;
    int enemyLP;

    const Card* playerHand[MAX_HAND_CARDS];
    int playerHandCount;
    const Card* enemyHand[MAX_HAND_CARDS];
    int enemyHandCount;

    // Bộ bài (Deck) mỗi bên 40 lá
    const Card* playerDeck[DECK_SIZE];
    int playerDeckCount;
    const Card* enemyDeck[DECK_SIZE];
    int enemyDeckCount;

    Slot playerAtkRow[5];
    Slot playerDefRow[5];
    Slot enemyAtkRow[5];
    Slot enemyDefRow[5];
    
    Rectangle playerHandRects[MAX_HAND_CARDS];

    int selectedCardIndexInHand; 
    bool isPlayerTurn;
    
    // Quản lý luật YGO
    int totalTurnCount;              // Đếm tổng số lượt
    bool hasNormalSummonedThisTurn;  // Cờ giới hạn Normal Summon 1 lần/lượt

    GamePhase currentPhase;
    Slot* selectedAttacker; 

    Rectangle btnNextRect;
    Rectangle btnExitRect;

    const Card* hoveredCard; 
    bool hoveredCardIsEnemy;

    // Hiệu ứng rút bài
    DrawAnimState drawAnim;
    
    // Status
    int gameStatus; // 0 = playing, 1 = win, 2 = lose

} GameState;

typedef enum {
    SCREEN_MENU = 0,
    SCREEN_AI,
    SCREEN_PVP,
    SCREEN_STORY,
    SCREEN_SETTINGS
} GameScreen;

#endif // GAME_STRUCTS_H