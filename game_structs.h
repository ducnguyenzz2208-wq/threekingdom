#ifndef GAME_STRUCTS_H
#define GAME_STRUCTS_H

#include "raylib.h"
#include <stdbool.h>

extern bool isEnglishMode;

#define MAX_HAND_CARDS 10
#define DECK_SIZE 40

typedef enum { TYPE_VO, TYPE_VAN } CardType;
typedef enum { KINGDOM_THUC, KINGDOM_NGUY, KINGDOM_NGO, KINGDOM_QUAN, KINGDOM_TAN } Kingdom;

typedef struct {
  int id;
  char name_vn[50];
  char name_en[50];
  int stars;
  CardType type;
  Kingdom kingdom;
  int atk;
  int def;
  char imgPath[100];
  char desc_vn[255];
  char desc_en[255];
  Texture2D texture;
} Card;

typedef struct {
  bool active;
  float timer;
  float duration;
  Vector2 startPos;
  Vector2 endPos;
  float startW, startH;
  float endW, endH;
  const Card *card;
  bool showFront;
  float flipTimer;
} DrawAnimState;

typedef enum {
  PHASE_PREPARATION = 0,
  PHASE_DRAW,
  PHASE_STANDBY,
  PHASE_MAIN_1,
  PHASE_BATTLE,
  PHASE_MAIN_2,
  PHASE_END
} GamePhase;

typedef struct {
  const Card *card;
  bool isEmpty;
  Rectangle rect;
  bool isDefending;
  bool hasAttacked;
  bool summonedThisTurn;
  bool positionChangedThisTurn; // ĐÃ THÊM: Cờ khóa đổi tư thế nhiều lần trong 1
                                // lượt
} Slot;

typedef struct {
  int playerLP;
  int enemyLP;

  const Card *playerHand[MAX_HAND_CARDS];
  int playerHandCount;
  const Card *enemyHand[MAX_HAND_CARDS];
  int enemyHandCount;

  const Card *playerDeck[DECK_SIZE];
  int playerDeckCount;
  const Card *enemyDeck[DECK_SIZE];
  int enemyDeckCount;

  Slot playerAtkRow[5];
  Slot playerDefRow[5];
  Slot enemyAtkRow[5];
  Slot enemyDefRow[5];

  Rectangle playerHandRects[MAX_HAND_CARDS];

  int selectedCardIndexInHand;
  bool isPlayerTurn;

  int totalTurnCount;
  int normalSummonsThisTurn;

  GamePhase currentPhase;
  Slot *selectedAttacker;

  Rectangle btnNextRect;
  Rectangle btnExitRect;
  Rectangle btnDiscardRect;

  const Card *hoveredCard;
  bool hoveredCardIsEnemy;

  DrawAnimState drawAnim;
  int gameStatus;

  bool isRollingDice;
  int playerDiceValue;
  int enemyDiceValue;
  float diceTimer;

} GameState;

typedef enum {
  SCREEN_MENU = 0,
  SCREEN_AI,
  SCREEN_PVP,
  SCREEN_STORY,
  SCREEN_SETTINGS,
  SCREEN_SHOP,
  SCREEN_ARENA_SELECT
} GameScreen;

#endif // GAME_STRUCTS_H