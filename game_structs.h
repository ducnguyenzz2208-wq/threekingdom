#ifndef GAME_STRUCTS_H
#define GAME_STRUCTS_H

extern bool isEnglishMode;

#include "raylib.h"
#include <stdbool.h>

#define MAX_HAND_CARDS 10
#define DECK_SIZE 40

typedef enum { TYPE_VO, TYPE_VAN } CardType;
typedef enum { KINGDOM_THUC, KINGDOM_NGUY, KINGDOM_NGO, KINGDOM_QUAN } Kingdom;

typedef struct {
  int id;
  char name_vn[50]; // Tên Tiếng Việt không dấu
  char name_en[50]; // Tên Tiếng Anh (Pinyin)
  int stars;
  CardType type;
  Kingdom kingdom;
  int atk;
  int def;
  char imgPath[100];
  char desc_vn[255]; // Cốt truyện Tiếng Việt không dấu
  char desc_en[255]; // Cốt truyện Tiếng Anh
  Texture2D texture; // Texture lưu trong bộ nhớ GPU để render bằng raylib
} Card;

typedef enum { PHASE_MAIN = 0, PHASE_BATTLE, PHASE_END } GamePhase;

typedef struct {
  const Card *card; // Con trỏ trỏ tới dữ liệu gốc trong cardDb
  bool isEmpty;
  Rectangle rect;   // Khu vực để render và bắt sự kiện click trên màn hình
  bool isDefending; // Xoay ngang
  bool hasAttacked; // Đã đánh trong lượt này chưa
} Slot;

typedef struct {
  int playerLP;
  int enemyLP;

  // Bộ bài và Tay bài
  const Card *playerHand[MAX_HAND_CARDS];
  int playerHandCount;
  const Card *enemyHand[MAX_HAND_CARDS];
  int enemyHandCount;

  // Sân đấu: Hàng công và hàng thủ
  Slot playerAtkRow[5];
  Slot playerDefRow[5];
  Slot enemyAtkRow[5];
  Slot enemyDefRow[5];

  // Rectangles cho bài trên tay để tiện check click
  Rectangle playerHandRects[MAX_HAND_CARDS];

  // Trạng thái thao tác
  int selectedCardIndexInHand; // -1 nếu chưa chọn bài nào
  bool isPlayerTurn;
  GamePhase currentPhase;
  Slot *selectedAttacker; // Lá bài phe ta đang chọn để đánh

  // Bảng thông tin (Left Panel) buttons
  Rectangle btnNextRect;
  Rectangle btnExitRect;

  // Hover logic
  const Card
      *hoveredCard; // Lá bài đang được trỏ chuột vào (trên sân hoặc trên tay)
  bool hoveredCardIsEnemy;

} GameState;

typedef enum {
  SCREEN_MENU = 0,
  SCREEN_AI,
  SCREEN_PVP,
  SCREEN_STORY
} GameScreen;

#endif // GAME_STRUCTS_H