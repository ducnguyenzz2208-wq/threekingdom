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

} GameState;

typedef enum {
    SCREEN_MENU = 0,
    SCREEN_AI,
    SCREEN_PVP,
    SCREEN_STORY
} GameScreen;

#endif // GAME_STRUCTS_H