#include "update.h"
#include "game_data.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

// Hàm tự động đếm và ăn quái vật hiến tế (Tribute)
static bool CheckAndConsumeTributes(GameState *state, int requiredTributes) {
  if (requiredTributes == 0)
    return true;

  int availableTributes = 0;
  Slot *tributeTargets[2] = {NULL, NULL}; // Tối đa hiến 2 quái

  // Quét hàng công
  for (int i = 0; i < 5 && availableTributes < requiredTributes; i++) {
    if (!state->playerAtkRow[i].isEmpty) {
      tributeTargets[availableTributes++] = &state->playerAtkRow[i];
    }
  }
  // Quét tiếp hàng thủ nếu chưa đủ
  for (int i = 0; i < 5 && availableTributes < requiredTributes; i++) {
    if (!state->playerDefRow[i].isEmpty) {
      tributeTargets[availableTributes++] = &state->playerDefRow[i];
    }
  }

  if (availableTributes >= requiredTributes) {
    for (int i = 0; i < requiredTributes; i++) {
      tributeTargets[i]->isEmpty = true;
      tributeTargets[i]->card = NULL;
    }
    return true;
  }

  return false; // Không đủ quái
}

static void PlaceCardInSlot(GameState *state, Slot *slot) {
  if (state->selectedCardIndexInHand >= 0 && slot->isEmpty) {
    slot->card = state->playerHand[state->selectedCardIndexInHand];
    slot->isEmpty = false;
    slot->hasAttacked = false;
    slot->summonedThisTurn = true; // Khóa tư thế

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
    int def = defender->card->def;
    if (atk > def) {
      defender->isEmpty = true;
      defender->card = NULL;
    } else if (atk < def) {
      state->playerLP -= (def - atk);
    }
  } else {
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
      defender->isEmpty = true;
      defender->card = NULL;
      attacker->isEmpty = true;
      attacker->card = NULL;
    }
  }

  attacker->hasAttacked = true;
  state->selectedAttacker = NULL;
}

// Hàm bắt đầu hiệu ứng rút bài
static void StartDrawAnimation(GameState *state, const Card *card) {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();
  int leftW = screenW * 0.25;
  if (leftW < 300)
    leftW = 300;
  int rightW = screenW - leftW;
  int handAreaH = screenH * 0.22;

  // Vị trí deck (góc phải dưới)
  int handCardH = handAreaH - 25;
  int handCardW = handCardH / 1.4;
  int deckCardW = handCardW * 0.7;
  int deckCardH = deckCardW * 1.4;
  int handY = screenH - handAreaH + 10;
  int deckX = screenW - deckCardW - 15;
  int deckY = handY + (handCardH - deckCardH) / 2;

  // Vị trí đích (giữa vùng tay bài)
  int endX = leftW + rightW / 2 - handCardW / 2;
  int endY = handY;

  state->drawAnim.active = true;
  state->drawAnim.timer = 0.0f;
  state->drawAnim.duration = 0.45f;
  state->drawAnim.startPos = (Vector2){deckX, deckY};
  state->drawAnim.endPos = (Vector2){endX, endY};
  state->drawAnim.startW = deckCardW;
  state->drawAnim.startH = deckCardH;
  state->drawAnim.endW = handCardW;
  state->drawAnim.endH = handCardH;
  state->drawAnim.card = card;
  state->drawAnim.showFront = false;
  state->drawAnim.flipTimer = 0.0f;
}

// Cập nhật hiệu ứng rút bài mỗi frame
static void UpdateDrawAnimation(GameState *state) {
  if (!state->drawAnim.active)
    return;

  float dt = GetFrameTime();
  state->drawAnim.timer += dt;
  state->drawAnim.flipTimer += dt;

  // Lật bài ở 40% thời gian animation
  if (state->drawAnim.flipTimer >= state->drawAnim.duration * 0.4f) {
    state->drawAnim.showFront = true;
  }

  // Kết thúc animation
  if (state->drawAnim.timer >= state->drawAnim.duration) {
    state->drawAnim.active = false;
    // Thêm bài vào tay khi animation xong
    if (state->playerHandCount < MAX_HAND_CARDS) {
      state->playerHand[state->playerHandCount++] = state->drawAnim.card;
    }
    state->drawAnim.card = NULL;
    // Chuyển sang Standby Phase
    state->currentPhase = PHASE_STANDBY;
  }
}

static void DoEnemyTurn(GameState *state) {
  // Enemy rút 1 lá từ deck
  if (state->enemyDeckCount > 0 && state->enemyHandCount < MAX_HAND_CARDS) {
    state->enemyDeckCount--;
    state->enemyHand[state->enemyHandCount++] =
        state->enemyDeck[state->enemyDeckCount];
  }

  // Enemy triệu hồi 1 quái từ tay
  for (int i = 0; i < 5; i++) {
    if (state->enemyAtkRow[i].isEmpty && state->enemyHandCount > 0) {
      state->enemyHandCount--;
      state->enemyAtkRow[i].card = state->enemyHand[state->enemyHandCount];
      state->enemyAtkRow[i].isEmpty = false;
      state->enemyAtkRow[i].hasAttacked = false;
      state->enemyAtkRow[i].isDefending = false;
      break;
    }
  }

  for (int i = 0; i < 5; i++) {
    if (!state->enemyAtkRow[i].isEmpty) {
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
        state->playerLP -= state->enemyAtkRow[i].card->atk;
      }
    }
  }

  state->currentPhase = PHASE_DRAW;
  state->isPlayerTurn = true;

  for (int i = 0; i < 5; i++) {
    state->playerAtkRow[i].hasAttacked = false;
  }
}

void UpdateGameplay(GameState *state) {
  if (state->gameStatus != 0) return;
  if (state->playerLP <= 0) {
      state->gameStatus = 2;
      return;
  }
  if (state->enemyLP <= 0) {
      state->gameStatus = 1;
      return;
  }

  // Cập nhật animation rút bài (luôn chạy dù đang ở phase nào)
  UpdateDrawAnimation(state);

  // Nếu đang chạy animation thì block mọi input
  if (state->drawAnim.active)
    return;

  if (!state->isPlayerTurn) {
    DoEnemyTurn(state);
    return;
  }

  // Tự động chạy logic Draw Phase
  if (state->currentPhase == PHASE_DRAW) {
    if (state->totalTurnCount > 1) {
      if (state->playerHandCount < MAX_HAND_CARDS &&
          state->playerDeckCount > 0) {
        // Rút từ deck và bắt đầu animation
        state->playerDeckCount--;
        const Card *drawnCard = state->playerDeck[state->playerDeckCount];
        StartDrawAnimation(state, drawnCard);
        // KHÔNG chuyển phase ở đây - sẽ chuyển khi animation kết thúc
        return;
      }
    }
    state->currentPhase = PHASE_STANDBY;
    return;
  }

  // Tự động bỏ qua Standby Phase (Chưa có Trap/Quick-Play Spell)
  if (state->currentPhase == PHASE_STANDBY) {
    state->currentPhase = PHASE_MAIN_1;
    return;
  }

  Vector2 mousePos = GetMousePosition();
  state->hoveredCard = NULL;
  state->hoveredCardIsEnemy = false;

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

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    if (CheckCollisionPointRec(mousePos, state->btnNextRect)) {
      switch (state->currentPhase) {
      case PHASE_MAIN_1:
        if (state->totalTurnCount == 1) {
          // Lượt 1 không có Battle Phase và Main Phase 2 -> Chuyển thẳng sang
          // End Phase
          state->currentPhase = PHASE_END;
          state->isPlayerTurn = false;
          state->totalTurnCount++;
          state->hasNormalSummonedThisTurn = false;
          for (int i = 0; i < 5; i++) {
            state->playerAtkRow[i].summonedThisTurn = false;
            state->playerDefRow[i].summonedThisTurn = false;
          }
        } else {
          // Từ lượt 2 trở đi, tiến vào Battle Phase bình thường
          state->currentPhase = PHASE_BATTLE;
        }
        state->selectedCardIndexInHand = -1;
        break;

      case PHASE_BATTLE:
        state->currentPhase = PHASE_MAIN_2;
        state->selectedAttacker = NULL;
        break;

      case PHASE_MAIN_2:
        state->currentPhase = PHASE_END;
        state->isPlayerTurn = false;
        state->totalTurnCount++;
        state->hasNormalSummonedThisTurn = false;
        for (int i = 0; i < 5; i++) {
          state->playerAtkRow[i].summonedThisTurn = false;
          state->playerDefRow[i].summonedThisTurn = false;
        }
        break;
      default:
        break;
      }
      return;
    }

    if (state->currentPhase == PHASE_MAIN_1 ||
        state->currentPhase == PHASE_MAIN_2) {
      bool clickedHand = false;
      for (int i = 0; i < state->playerHandCount; i++) {
        if (CheckCollisionPointRec(mousePos, state->playerHandRects[i])) {
          state->selectedCardIndexInHand = i;
          clickedHand = true;
          break;
        }
      }

      if (!clickedHand && state->selectedCardIndexInHand >= 0) {
        if (state->hasNormalSummonedThisTurn) {
          state->selectedCardIndexInHand = -1;
          return;
        }

        const Card *cardToSummon =
            state->playerHand[state->selectedCardIndexInHand];
        int reqTributes = 0;
        if (cardToSummon->stars >= 7)
          reqTributes = 2;
        else if (cardToSummon->stars >= 5)
          reqTributes = 1;

        bool placed = false;

        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect)) {
            if (CheckAndConsumeTributes(state, reqTributes)) {
              PlaceCardInSlot(state, &state->playerAtkRow[i]);
              state->hasNormalSummonedThisTurn = true;
              placed = true;
            }
            break;
          }
        }
        if (!placed) {
          for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect)) {
              if (CheckAndConsumeTributes(state, reqTributes)) {
                PlaceCardInSlot(state, &state->playerDefRow[i]);
                state->hasNormalSummonedThisTurn = true;
                placed = true;
              }
              break;
            }
          }
        }

        if (!placed)
          state->selectedCardIndexInHand = -1;
      }
    } else if (state->currentPhase == PHASE_BATTLE) {
      if (state->selectedAttacker == NULL) {
        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect) &&
              !state->playerAtkRow[i].isEmpty &&
              !state->playerAtkRow[i].hasAttacked) {
            state->selectedAttacker = &state->playerAtkRow[i];
            break;
          }
        }
      } else {
        bool attacked = false;
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

        if (!attacked) {
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