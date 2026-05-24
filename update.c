#include "update.h"
#include "game_data.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

// Hàm AI chuẩn bị bài (Tung hết bài trên tay ra sân)
static void DoEnemyPreparation(GameState *state) {
  for (int i = 0; i < state->enemyHandCount; i++) {
    bool placed = false;
    for (int j = 0; j < 5; j++) {
      if (state->enemyDefRow[j].isEmpty) {
        state->enemyDefRow[j].card = state->enemyHand[i];
        state->enemyDefRow[j].isEmpty = false;
        state->enemyDefRow[j].hasAttacked = false;
        state->enemyDefRow[j].isDefending = false;
        state->enemyDefRow[j].summonedThisTurn = true;
        state->enemyDefRow[j].positionChangedThisTurn = false;
        placed = true;
        break;
      }
    }
    if (!placed) {
      for (int j = 0; j < 5; j++) {
        if (state->enemyAtkRow[j].isEmpty) {
          state->enemyAtkRow[j].card = state->enemyHand[i];
          state->enemyAtkRow[j].isEmpty = false;
          state->enemyAtkRow[j].hasAttacked = false;
          state->enemyAtkRow[j].isDefending = true;
          state->enemyAtkRow[j].summonedThisTurn = true;
          state->enemyAtkRow[j].positionChangedThisTurn = false;
          break;
        }
      }
    }
  }
  state->enemyHandCount = 0;
}

// Hàm tự động đếm và ăn quái vật hiến tế TỪ MONSTER ZONE
static bool CheckAndConsumeTributes(GameState *state, int requiredTributes) {
  if (requiredTributes == 0)
    return true;

  int availableTributes = 0;
  Slot *tributeTargets[2] = {NULL, NULL};

  // Quét Monster Zone (Hàng giữa - playerDefRow)
  for (int i = 0; i < 5 && availableTributes < requiredTributes; i++) {
    if (!state->playerDefRow[i].isEmpty) {
      tributeTargets[availableTributes++] = &state->playerDefRow[i];
    }
  }
  
  // Quét thêm hàng sau (playerAtkRow)
  for (int i = 0; i < 5 && availableTributes < requiredTributes; i++) {
    if (!state->playerAtkRow[i].isEmpty) {
      tributeTargets[availableTributes++] = &state->playerAtkRow[i];
    }
  }

  if (availableTributes >= requiredTributes) {
    for (int i = 0; i < requiredTributes; i++) {
      tributeTargets[i]->isEmpty = true;
      tributeTargets[i]->card = NULL;
    }
    return true;
  }
  return false;
}

// Hàm đặt bài
static void PlaceCardInSlot(GameState *state, Slot *slot, bool isSetMode) {
  if (state->selectedCardIndexInHand >= 0 && slot->isEmpty) {
    slot->card = state->playerHand[state->selectedCardIndexInHand];
    slot->isEmpty = false;
    slot->hasAttacked = false;
    slot->summonedThisTurn = true;
    slot->positionChangedThisTurn = false;
    slot->isDefending = isSetMode; // Đặt ngửa Công hoặc Úp Thủ

    for (int i = state->selectedCardIndexInHand; i < state->playerHandCount - 1;
         i++) {
      state->playerHand[i] = state->playerHand[i + 1];
    }
    state->playerHandCount--;
    state->selectedCardIndexInHand = -1;
  }
}

// FIX LỖI SÁT THƯƠNG: Tính sát thương chuẩn ai đâm ai
static void PerformCombat(GameState *state, Slot *attacker, Slot *defender,
                          bool isPlayerAttacking) {
  if (!attacker || !defender || attacker->isEmpty || defender->isEmpty)
    return;

  int atk = attacker->card->atk;

  if (defender->isDefending) {
    int def = defender->card->def;
    if (atk > def) {
      defender->isEmpty = true;
      defender->card = NULL;
    } else if (atk < def) {
      // Đâm vào quái thủ to hơn -> Tự mất máu
      if (isPlayerAttacking)
        state->playerLP -= (def - atk);
      else
        state->enemyLP -= (def - atk);
    }
  } else {
    int def_atk = defender->card->atk;
    if (atk > def_atk) {
      defender->isEmpty = true;
      defender->card = NULL;
      if (isPlayerAttacking)
        state->enemyLP -= (atk - def_atk);
      else
        state->playerLP -= (atk - def_atk);
    } else if (atk < def_atk) {
      attacker->isEmpty = true;
      attacker->card = NULL;
      if (isPlayerAttacking)
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

// ... [Giữ nguyên StartDrawAnimation và UpdateDrawAnimation] ...
static void StartDrawAnimation(GameState *state, const Card *card) {
  int screenW = GetScreenWidth();
  int screenH = GetScreenHeight();
  int leftW = screenW * 0.25;
  if (leftW < 300)
    leftW = 300;
  int rightW = screenW - leftW;
  int handAreaH = screenH * 0.22;

  int handCardH = handAreaH - 25;
  int handCardW = handCardH / 1.4;
  int deckCardW = handCardW * 0.7;
  int deckCardH = deckCardW * 1.4;
  int handY = screenH - handAreaH + 10;
  int deckX = screenW - deckCardW - 15;
  int deckY = handY + (handCardH - deckCardH) / 2;

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

static void UpdateDrawAnimation(GameState *state) {
  if (!state->drawAnim.active)
    return;

  float dt = GetFrameTime();
  state->drawAnim.timer += dt;
  state->drawAnim.flipTimer += dt;

  if (state->drawAnim.flipTimer >= state->drawAnim.duration * 0.4f) {
    state->drawAnim.showFront = true;
  }

  if (state->drawAnim.timer >= state->drawAnim.duration) {
    state->drawAnim.active = false;
    if (state->playerHandCount < MAX_HAND_CARDS) {
      state->playerHand[state->playerHandCount++] = state->drawAnim.card;
    }
    state->drawAnim.card = NULL;
    state->currentPhase = PHASE_STANDBY;
  }
}

static void DoEnemyTurn(GameState *state) {
  if (state->enemyDeckCount > 0 && state->enemyHandCount < MAX_HAND_CARDS) {
    state->enemyDeckCount--;
    state->enemyHand[state->enemyHandCount++] =
        state->enemyDeck[state->enemyDeckCount];
  }

  // Quái thú địch rơi vào Enemy Monster Zone (enemyDefRow)
  for (int i = 0; i < 5; i++) {
    if (state->enemyDefRow[i].isEmpty && state->enemyHandCount > 0) {
      state->enemyHandCount--;
      state->enemyDefRow[i].card = state->enemyHand[state->enemyHandCount];
      state->enemyDefRow[i].isEmpty = false;
      state->enemyDefRow[i].hasAttacked = false;
      state->enemyDefRow[i].isDefending = false;
      state->enemyDefRow[i].summonedThisTurn = true;
      state->enemyDefRow[i].positionChangedThisTurn = false;
      break;
    }
  }

  // Địch tấn công
  Slot *enemyAttackers[10];
  int attackerCount = 0;
  for (int i = 0; i < 5; i++) {
    if (!state->enemyDefRow[i].isEmpty && !state->enemyDefRow[i].isDefending) {
      enemyAttackers[attackerCount++] = &state->enemyDefRow[i];
    }
    if (!state->enemyAtkRow[i].isEmpty && !state->enemyAtkRow[i].isDefending) {
      enemyAttackers[attackerCount++] = &state->enemyAtkRow[i];
    }
  }

  for (int k = 0; k < attackerCount; k++) {
    Slot *attacker = enemyAttackers[k];
    if (attacker->isEmpty || attacker->hasAttacked) continue;

    Slot *target = NULL;
    // Ưu tiên phá hàng Thủ (Frontline - playerAtkRow) trước
    for (int j = 0; j < 5; j++) {
      if (!state->playerAtkRow[j].isEmpty) {
        target = &state->playerAtkRow[j];
        break;
      }
    }
    // Nếu hàng Thủ trống, đánh hàng Công (Backline - playerDefRow)
    if (!target) {
      for (int j = 0; j < 5; j++) {
        if (!state->playerDefRow[j].isEmpty) {
          target = &state->playerDefRow[j];
          break;
        }
      }
    }

    if (target) {
      PerformCombat(state, attacker, target, false);
    } else {
      state->playerLP -= attacker->card->atk;
      attacker->hasAttacked = true;
    }
  }

  state->currentPhase = PHASE_DRAW;
  state->isPlayerTurn = true;

  // Reset cờ chiến đấu cho Player khi sang Turn mới
  for (int i = 0; i < 5; i++) {
    state->playerDefRow[i].hasAttacked = false;
    state->playerDefRow[i].summonedThisTurn = false;
    state->playerDefRow[i].positionChangedThisTurn = false;
    state->playerAtkRow[i].hasAttacked = false;
    state->playerAtkRow[i].summonedThisTurn = false;
    state->playerAtkRow[i].positionChangedThisTurn = false;
  }
}

void UpdateGameplay(GameState *state) {
  if (state->gameStatus != 0)
    return;
  if (state->playerLP <= 0) {
    state->gameStatus = 2;
    return;
  }
  if (state->enemyLP <= 0) {
    state->gameStatus = 1;
    return;
  }

  UpdateDrawAnimation(state);
  if (state->drawAnim.active)
    return;

  if (state->isRollingDice) {
    state->diceTimer += GetFrameTime();
    
    if ((int)(state->diceTimer * 10) % 2 == 0) {
      state->playerDiceValue = GetRandomValue(1, 6);
      state->enemyDiceValue = GetRandomValue(1, 6);
    }
    
    if (state->diceTimer >= 2.0f) {
      state->isRollingDice = false;
      state->diceTimer = 0.0f;
      while (state->playerDiceValue == state->enemyDiceValue) {
        state->playerDiceValue = GetRandomValue(1, 6);
        state->enemyDiceValue = GetRandomValue(1, 6);
      }
      
      if (state->playerDiceValue > state->enemyDiceValue) {
        state->isPlayerTurn = true;
      } else {
        state->isPlayerTurn = false;
      }
      state->currentPhase = PHASE_MAIN_1; 
      state->totalTurnCount = 1;
    }
    return;
  }

  if (!state->isPlayerTurn) {
    DoEnemyTurn(state);
    return;
  }

  if (state->currentPhase == PHASE_DRAW) {
    if (state->totalTurnCount > 1) {
      if (state->playerHandCount < MAX_HAND_CARDS &&
          state->playerDeckCount > 0) {
        state->playerDeckCount--;
        const Card *drawnCard = state->playerDeck[state->playerDeckCount];
        StartDrawAnimation(state, drawnCard);
        return;
      }
    }
    state->currentPhase = PHASE_STANDBY;
    return;
  }

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
      case PHASE_PREPARATION:
        state->selectedCardIndexInHand = -1;
        DoEnemyPreparation(state);
        state->isRollingDice = true;
        state->diceTimer = 0.0f;
        break;
        
      case PHASE_MAIN_1:
        if (state->totalTurnCount == 1) {
          state->currentPhase = PHASE_END;
          state->isPlayerTurn = false;
          state->totalTurnCount++;
          state->hasNormalSummonedThisTurn = false;
          for (int i = 0; i < 5; i++) {
            state->enemyDefRow[i].summonedThisTurn = false;
            state->enemyDefRow[i].positionChangedThisTurn = false;
            state->enemyAtkRow[i].summonedThisTurn = false;
            state->enemyAtkRow[i].positionChangedThisTurn = false;
          }
        } else {
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
          state->enemyDefRow[i].summonedThisTurn = false;
          state->enemyDefRow[i].positionChangedThisTurn = false;
          state->enemyAtkRow[i].summonedThisTurn = false;
          state->enemyAtkRow[i].positionChangedThisTurn = false;
        }
        break;
      default:
        break;
      }
      return;
    }

    if (state->currentPhase == PHASE_PREPARATION ||
        state->currentPhase == PHASE_MAIN_1 ||
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
        if (state->hasNormalSummonedThisTurn && state->currentPhase != PHASE_PREPARATION) {
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
          // Normal Summon vào Monster Zone (Hàng thứ 3 từ trên xuống)
          if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect)) {
            if (CheckAndConsumeTributes(state, reqTributes)) {
              PlaceCardInSlot(state, &state->playerDefRow[i],
                              false); // Đặt thế Công
              if (state->currentPhase != PHASE_PREPARATION) {
                state->hasNormalSummonedThisTurn = true;
              }
              placed = true;
            }
            break;
          }
          // Đặt bài Phép/Bẫy (Hàng sát mép màn hình dưới cùng)
          if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect)) {
            PlaceCardInSlot(state, &state->playerAtkRow[i], true); // Úp thế Thủ
            placed = true;
            break;
          }
        }
        if (!placed)
          state->selectedCardIndexInHand = -1;
      }
    } else if (state->currentPhase == PHASE_BATTLE) {
      if (state->selectedAttacker == NULL) {
        // Chỉ được tấn công bằng quái ở thế Công
        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect) &&
              !state->playerDefRow[i].isEmpty &&
              !state->playerDefRow[i].isDefending &&
              !state->playerDefRow[i].hasAttacked) {
            state->selectedAttacker = &state->playerDefRow[i];
            break;
          }
          if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect) &&
              !state->playerAtkRow[i].isEmpty &&
              !state->playerAtkRow[i].isDefending &&
              !state->playerAtkRow[i].hasAttacked) {
            state->selectedAttacker = &state->playerAtkRow[i];
            break;
          }
        }
      } else {
        bool enemyThủHasCards = false;
        for (int i = 0; i < 5; i++) {
          if (!state->enemyAtkRow[i].isEmpty) enemyThủHasCards = true;
        }

        bool attacked = false;
        // Đánh vào Frontline Thủ (Enemy Atk Row)
        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->enemyAtkRow[i].rect) &&
              !state->enemyAtkRow[i].isEmpty) {
            PerformCombat(state, state->selectedAttacker,
                          &state->enemyAtkRow[i], true);
            attacked = true;
            break;
          }
        }

        // Đánh vào Backline Công (Enemy Def Row) - Chỉ được nếu Frontline Thủ đã bị phá hết
        if (!attacked && !enemyThủHasCards) {
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
          bool enemyHasAnyCards = false;
          for (int i = 0; i < 5; i++) {
            if (!state->enemyDefRow[i].isEmpty || !state->enemyAtkRow[i].isEmpty) {
              enemyHasAnyCards = true;
            }
          }

          if (!enemyHasAnyCards) {
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

  // ĐÃ SỬA: CHUỘT PHẢI ĐỂ HỦY CHỌN HOẶC ĐỔI TƯ THẾ CHIẾN ĐẤU
  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    if (state->selectedCardIndexInHand >= 0 ||
        state->selectedAttacker != NULL) {
      state->selectedCardIndexInHand = -1;
      state->selectedAttacker = NULL;
    } else {
      if (state->currentPhase == PHASE_PREPARATION ||
          state->currentPhase == PHASE_MAIN_1 ||
          state->currentPhase == PHASE_MAIN_2) {
        for (int i = 0; i < 5; i++) {
          if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect) &&
              !state->playerDefRow[i].isEmpty) {
            Slot *slot = &state->playerDefRow[i];
            if (!slot->summonedThisTurn && !slot->positionChangedThisTurn &&
                !slot->hasAttacked) {
              slot->isDefending = !slot->isDefending; 
              slot->positionChangedThisTurn = true;   
            }
            break;
          }
          if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect) &&
              !state->playerAtkRow[i].isEmpty) {
            Slot *slot = &state->playerAtkRow[i];
            if (!slot->summonedThisTurn && !slot->positionChangedThisTurn &&
                !slot->hasAttacked) {
              slot->isDefending = !slot->isDefending; 
              slot->positionChangedThisTurn = true;   
            }
            break;
          }
        }
      }
    }
  }
}