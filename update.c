#include "update.h"
#include "game_data.h"
#include <stddef.h>
#include <stdlib.h>

static void PlaceCardInSlot(GameState *state, Slot *slot) {
  if (state->selectedCardIndexInHand >= 0 && slot->isEmpty) {
    slot->card = state->playerHand[state->selectedCardIndexInHand];
    slot->isEmpty = false;
    slot->hasAttacked = false;

    for (int i = state->selectedCardIndexInHand; i < state->playerHandCount - 1;
         i++) {
      state->playerHand[i] = state->playerHand[i + 1];
    }
    state->playerHandCount--;
    state->selectedCardIndexInHand = -1;
  }
}

static void PerformCombat(GameState *state, Slot *attacker, Slot *defender,
                          bool isEnemyDefending) {
  if (!attacker || !defender || attacker->isEmpty || defender->isEmpty)
    return;

  int atk = attacker->card->atk;

  if (defender->isDefending) {
    // Tấn công quái phòng thủ
    int def = defender->card->def;
    if (atk > def) {
      defender->isEmpty = true; // Phá hủy
      defender->card = NULL;
    } else if (atk < def) {
      // Phe tấn công mất máu
      state->playerLP -= (def - atk);
    }
  } else {
    // Tấn công quái tấn công
    int def_atk = defender->card->atk;
    if (atk > def_atk) {
      defender->isEmpty = true;
      defender->card = NULL;
      if (isEnemyDefending)
        state->enemyLP -= (atk - def_atk);
      else
        state->playerLP -= (atk - def_atk);
    } else if (atk < def_atk) {
      attacker->isEmpty = true;
      attacker->card = NULL;
      if (isEnemyDefending)
        state->playerLP -= (def_atk - atk);
      else
        state->enemyLP -= (def_atk - atk);
    } else {
      // Hòa, cả 2 chết
      defender->isEmpty = true;
      defender->card = NULL;
      attacker->isEmpty = true;
      attacker->card = NULL;
    }
  }

  attacker->hasAttacked = true;
  state->selectedAttacker = NULL;
}

static void DoEnemyTurn(GameState *state) {
  // Rất căn bản: AI thử đặt 1 lá bài từ "tay" ra hàng Công
  // Hiện tại tay AI được giả lập là luôn đầy. Ta lấy random 1 thẻ từ DB.

  // AI đặt bài
  for (int i = 0; i < 5; i++) {
    if (state->enemyAtkRow[i].isEmpty) {
      state->enemyAtkRow[i].card = &cardDb[rand() % TOTAL_CARDS];
      state->enemyAtkRow[i].isEmpty = false;
      state->enemyAtkRow[i].hasAttacked = false;
      state->enemyAtkRow[i].isDefending = false;
      break; // Chỉ đặt 1 con
    }
  }

  // AI tấn công (Tất cả quái AI chưa tấn công sẽ lao vào quái Player)
  for (int i = 0; i < 5; i++) {
    if (!state->enemyAtkRow[i].isEmpty) {
      // Tìm mục tiêu (ưu tiên Atk Row)
      Slot *target = NULL;
      for (int j = 0; j < 5; j++) {
        if (!state->playerAtkRow[j].isEmpty) {
          target = &state->playerAtkRow[j];
          break;
        }
      }
      if (!target) {
        for (int j = 0; j < 5; j++) {
          if (!state->playerDefRow[j].isEmpty) {
            target = &state->playerDefRow[j];
            break;
          }
        }
      }

      if (target) {
        PerformCombat(state, &state->enemyAtkRow[i], target, false);
      } else {
        // Tấn công trực tiếp
        state->playerLP -= state->enemyAtkRow[i].card->atk;
      }
    }
  }

  // Chuyển lại cho Player
  state->currentPhase = PHASE_MAIN;
  state->isPlayerTurn = true;

  // Reset cờ tấn công phe ta
  for (int i = 0; i < 5; i++) {
    state->playerAtkRow[i].hasAttacked = false;
  }

  // Bốc 1 bài cho Player
  if (state->playerHandCount < 10) {
    state->playerHand[state->playerHandCount++] = &cardDb[rand() % TOTAL_CARDS];
  }
}

void UpdateGameplay(GameState *state) {
  if (!state->isPlayerTurn) {
    DoEnemyTurn(state);
    return;
  }

  Vector2 mousePos = GetMousePosition();

  state->hoveredCard = NULL;
  state->hoveredCardIsEnemy = false;

  // --- Hover logic ---
  for (int i = 0; i < state->playerHandCount; i++) {
    if (CheckCollisionPointRec(mousePos, state->playerHandRects[i])) {
      state->hoveredCard = state->playerHand[i];
      break;
    }
  }
  if (!state->hoveredCard) {
    for (int i = 0; i < 5; i++) {
      if (!state->playerAtkRow[i].isEmpty &&
          CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect)) {
        state->hoveredCard = state->playerAtkRow[i].card;
        break;
      }
      if (!state->playerDefRow[i].isEmpty &&
          CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect)) {
        state->hoveredCard = state->playerDefRow[i].card;
        break;
      }
    }
  }
  if (!state->hoveredCard) {
    for (int i = 0; i < 5; i++) {
      if (!state->enemyAtkRow[i].isEmpty &&
          CheckCollisionPointRec(mousePos, state->enemyAtkRow[i].rect)) {
        state->hoveredCard = state->enemyAtkRow[i].card;
        state->hoveredCardIsEnemy = true;
        break;
      }
      if (!state->enemyDefRow[i].isEmpty &&
          CheckCollisionPointRec(mousePos, state->enemyDefRow[i].rect)) {
        state->hoveredCard = state->enemyDefRow[i].card;
        state->hoveredCardIsEnemy = true;
        break;
      }
    }
  }

  // --- Click logic ---
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {

    // 1. Kiểm tra click vào nút bấm
    if (CheckCollisionPointRec(mousePos, state->btnNextRect)) {
      if (state->currentPhase == PHASE_MAIN) {
        state->currentPhase = PHASE_BATTLE;
        state->selectedCardIndexInHand = -1;
      } else if (state->currentPhase == PHASE_BATTLE) {
        state->currentPhase = PHASE_END;
        state->isPlayerTurn = false;
        state->selectedAttacker = NULL;
      }
      return;
    }

    // 2. Logic cho PHASE_MAIN
    if (state->currentPhase == PHASE_MAIN) {
      bool clickedHand = false;
      for (int i = 0; i < state->playerHandCount; i++) {
        if (CheckCollisionPointRec(mousePos, state->playerHandRects[i])) {
          state->selectedCardIndexInHand = i;
          clickedHand = true;
          break;
        }
      }

      if (!clickedHand && state->selectedCardIndexInHand >= 0) {
        bool placed = false;
        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect)) {
            PlaceCardInSlot(state, &state->playerAtkRow[i]);
            placed = true;
            break;
          }
        }
        if (!placed) {
          for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect)) {
              PlaceCardInSlot(state, &state->playerDefRow[i]);
              placed = true;
              break;
            }
          }
        }
        if (!placed)
          state->selectedCardIndexInHand = -1;
      }
    }

    // 3. Logic cho PHASE_BATTLE
    else if (state->currentPhase == PHASE_BATTLE) {
      // Chọn Attacker
      if (state->selectedAttacker == NULL) {
        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect) &&
              !state->playerAtkRow[i].isEmpty &&
              !state->playerAtkRow[i].hasAttacked) {
            state->selectedAttacker = &state->playerAtkRow[i];
            break;
          }
        }
      }
      // Đã có Attacker, chọn mục tiêu
      else {
        bool attacked = false;
        // Click kẻ địch
        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->enemyAtkRow[i].rect) &&
              !state->enemyAtkRow[i].isEmpty) {
            PerformCombat(state, state->selectedAttacker,
                          &state->enemyAtkRow[i], true);
            attacked = true;
            break;
          }
        }
        if (!attacked) {
          for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, state->enemyDefRow[i].rect) &&
                !state->enemyDefRow[i].isEmpty) {
              PerformCombat(state, state->selectedAttacker,
                            &state->enemyDefRow[i], true);
              attacked = true;
              break;
            }
          }
        }

        // Nếu click ra chỗ khác không phải địch, hoặc click trực tiếp (khi địch
        // không có quái)
        if (!attacked) {
          // Tấn công trực tiếp (Direct Attack)
          bool enemyHasCards = false;
          for (int i = 0; i < 5; i++)
            if (!state->enemyAtkRow[i].isEmpty ||
                !state->enemyDefRow[i].isEmpty)
              enemyHasCards = true;

          if (!enemyHasCards) {
            state->enemyLP -= state->selectedAttacker->card->atk;
            state->selectedAttacker->hasAttacked = true;
            state->selectedAttacker = NULL;
          } else {
            // Click trượt, bỏ chọn
            state->selectedAttacker = NULL;
          }
        }
      }
    }
  }

  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    state->selectedCardIndexInHand = -1;
    state->selectedAttacker = NULL;
  }
}
