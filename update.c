#include "update.h"
#include "game_data.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

// Hàm AI chuẩn bị bài (Tung hết bài trên tay ra sân)
static void DoEnemyPreparation(GameState *state) {
  int summonCount = 0;
  int i = 0;
  while (i < state->enemyHandCount && summonCount < 2) {
    bool placed = false;
    const Card *card = state->enemyHand[i];
    bool preferAttack = card->atk >= card->def;

    if (preferAttack) {
      for (int j = 0; j < 5; j++) {
        if (state->enemyDefRow[j].isEmpty) {
          state->enemyDefRow[j].card = card;
          state->enemyDefRow[j].isEmpty = false;
          state->enemyDefRow[j].hasAttacked = false;
          state->enemyDefRow[j].isDefending = false;
          state->enemyDefRow[j].summonedThisTurn = true;
          state->enemyDefRow[j].positionChangedThisTurn = false;
          placed = true;
          break;
        }
      }
    } else {
      for (int j = 0; j < 5; j++) {
        if (state->enemyAtkRow[j].isEmpty) {
          state->enemyAtkRow[j].card = card;
          state->enemyAtkRow[j].isEmpty = false;
          state->enemyAtkRow[j].hasAttacked = false;
          state->enemyAtkRow[j].isDefending = true;
          state->enemyAtkRow[j].summonedThisTurn = true;
          state->enemyAtkRow[j].positionChangedThisTurn = false;
          placed = true;
          break;
        }
      }
    }

    if (!placed) {
      // Place in whatever row is available if preferred row is full
      for (int j = 0; j < 5; j++) {
        if (state->enemyDefRow[j].isEmpty) {
          state->enemyDefRow[j].card = card;
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
            state->enemyAtkRow[j].card = card;
            state->enemyAtkRow[j].isEmpty = false;
            state->enemyAtkRow[j].hasAttacked = false;
            state->enemyAtkRow[j].isDefending = true;
            state->enemyAtkRow[j].summonedThisTurn = true;
            state->enemyAtkRow[j].positionChangedThisTurn = false;
            placed = true;
            break;
          }
        }
      }
    }

    if (placed) {
      // Remove from hand by shifting
      for (int k = i; k < state->enemyHandCount - 1; k++) {
        state->enemyHand[k] = state->enemyHand[k + 1];
      }
      state->enemyHandCount--;
      summonCount++;
    } else {
      // If board is full, try next card
      i++;
    }
  }
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
      if (isPlayerAttacking)
        state->enemyLP -= (atk - def);
      else
        state->playerLP -= (atk - def);
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

  // Quái thú địch rơi vào trận địa dựa trên chỉ số ATK/DEF
  if (state->enemyHandCount > 0) {
    const Card *cardToSummon = state->enemyHand[state->enemyHandCount - 1];
    bool preferAttack = cardToSummon->atk >= cardToSummon->def;
    bool placed = false;
    
    if (preferAttack) {
      for (int i = 0; i < 5; i++) {
        if (state->enemyDefRow[i].isEmpty) {
          state->enemyHandCount--;
          state->enemyDefRow[i].card = cardToSummon;
          state->enemyDefRow[i].isEmpty = false;
          state->enemyDefRow[i].hasAttacked = false;
          state->enemyDefRow[i].isDefending = false;
          state->enemyDefRow[i].summonedThisTurn = true;
          state->enemyDefRow[i].positionChangedThisTurn = false;
          placed = true;
          break;
        }
      }
    } else {
      for (int i = 0; i < 5; i++) {
        if (state->enemyAtkRow[i].isEmpty) {
          state->enemyHandCount--;
          state->enemyAtkRow[i].card = cardToSummon;
          state->enemyAtkRow[i].isEmpty = false;
          state->enemyAtkRow[i].hasAttacked = false;
          state->enemyAtkRow[i].isDefending = true;
          state->enemyAtkRow[i].summonedThisTurn = true;
          state->enemyAtkRow[i].positionChangedThisTurn = false;
          placed = true;
          break;
        }
      }
    }

    if (!placed) {
      // Đặt vào vị trí trống bất kỳ nếu hàng ưu tiên đã đầy
      for (int i = 0; i < 5; i++) {
        if (state->enemyDefRow[i].isEmpty) {
          state->enemyHandCount--;
          state->enemyDefRow[i].card = cardToSummon;
          state->enemyDefRow[i].isEmpty = false;
          state->enemyDefRow[i].hasAttacked = false;
          state->enemyDefRow[i].isDefending = false;
          state->enemyDefRow[i].summonedThisTurn = true;
          state->enemyDefRow[i].positionChangedThisTurn = false;
          placed = true;
          break;
        }
        if (state->enemyAtkRow[i].isEmpty) {
          state->enemyHandCount--;
          state->enemyAtkRow[i].card = cardToSummon;
          state->enemyAtkRow[i].isEmpty = false;
          state->enemyAtkRow[i].hasAttacked = false;
          state->enemyAtkRow[i].isDefending = true;
          state->enemyAtkRow[i].summonedThisTurn = true;
          state->enemyAtkRow[i].positionChangedThisTurn = false;
          placed = true;
          break;
        }
      }
    }
  }

  // Địch tấn công
  if (state->totalTurnCount > 1) {
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
    Slot *candidates[5];
    int candidateCount = 0;
    
    // Ưu tiên phá hàng Thủ (Frontline - playerAtkRow) trước
    for (int j = 0; j < 5; j++) {
      if (!state->playerAtkRow[j].isEmpty) {
        candidates[candidateCount++] = &state->playerAtkRow[j];
      }
    }
    
    // Nếu hàng Thủ trống, đánh hàng Công (Backline - playerDefRow)
    if (candidateCount == 0) {
      for (int j = 0; j < 5; j++) {
        if (!state->playerDefRow[j].isEmpty) {
          candidates[candidateCount++] = &state->playerDefRow[j];
        }
      }
    }

    if (candidateCount > 0) {
      int bestScore = -999999;
      target = candidates[0];
      int atk = attacker->card->atk;
      for (int i = 0; i < candidateCount; i++) {
        int score = 0;
        Slot *slot = candidates[i];
        if (slot->isDefending) {
          if (atk > slot->card->def) {
            // Ưu tiên quân thủ có thể tiêu diệt, DEF càng thấp điểm càng cao (để gây nhiều sát thương chênh lệch)
            score = 10000 - slot->card->def;
          } else {
            score = -1000 - (slot->card->def - atk);
          }
        } else {
          int def_atk = slot->card->atk;
          if (atk > def_atk) {
            score = 5000 + (atk - def_atk);
          } else {
            score = -5000 - (def_atk - atk);
          }
        }
        if (score > bestScore) {
          bestScore = score;
          target = slot;
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
  }

  // AI limits: Discard excess cards to maintain max 3 in hand and max 5 on field
  while (state->enemyHandCount > 3) {
    int worstIndex = 0;
    int minStats = state->enemyHand[0]->atk + state->enemyHand[0]->def;
    for (int i = 1; i < state->enemyHandCount; i++) {
      int stats = state->enemyHand[i]->atk + state->enemyHand[i]->def;
      if (stats < minStats) {
        minStats = stats;
        worstIndex = i;
      }
    }
    for (int i = worstIndex; i < state->enemyHandCount - 1; i++) {
      state->enemyHand[i] = state->enemyHand[i + 1];
    }
    state->enemyHandCount--;
  }

  int enemyFieldCount = 0;
  for (int i = 0; i < 5; i++) {
    if (!state->enemyAtkRow[i].isEmpty) enemyFieldCount++;
    if (!state->enemyDefRow[i].isEmpty) enemyFieldCount++;
  }
  while (enemyFieldCount > 5) {
    Slot *worstSlot = NULL;
    int minStats = 999999;
    for (int i = 0; i < 5; i++) {
      if (!state->enemyAtkRow[i].isEmpty) {
        int stats = state->enemyAtkRow[i].card->atk + state->enemyAtkRow[i].card->def;
        if (stats < minStats) {
          minStats = stats;
          worstSlot = &state->enemyAtkRow[i];
        }
      }
      if (!state->enemyDefRow[i].isEmpty) {
        int stats = state->enemyDefRow[i].card->atk + state->enemyDefRow[i].card->def;
        if (stats < minStats) {
          minStats = stats;
          worstSlot = &state->enemyDefRow[i];
        }
      }
    }
    if (worstSlot) {
      worstSlot->isEmpty = true;
      worstSlot->card = NULL;
      enemyFieldCount--;
    } else {
      break;
    }
  }

  state->currentPhase = PHASE_DRAW;
  state->isPlayerTurn = true;
  state->totalTurnCount++;

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
    // Calculate player counts and limit check
    int playerFieldCount = 0;
    for (int i = 0; i < 5; i++) {
      if (!state->playerAtkRow[i].isEmpty) playerFieldCount++;
      if (!state->playerDefRow[i].isEmpty) playerFieldCount++;
    }
    bool isEndTurnPhase = (state->currentPhase == PHASE_MAIN_2 || (state->currentPhase == PHASE_MAIN_1 && state->totalTurnCount == 1));
    bool exceedsLimit = (state->playerHandCount > 3 || playerFieldCount > 5);

    if (isEndTurnPhase && exceedsLimit) {
      if (CheckCollisionPointRec(mousePos, state->btnDiscardRect)) {
        if (state->selectedCardIndexInHand >= 0) {
          for (int i = state->selectedCardIndexInHand; i < state->playerHandCount - 1; i++) {
            state->playerHand[i] = state->playerHand[i + 1];
          }
          state->playerHandCount--;
          state->selectedCardIndexInHand = -1;
        } else if (state->selectedAttacker != NULL) {
          state->selectedAttacker->isEmpty = true;
          state->selectedAttacker->card = NULL;
          state->selectedAttacker = NULL;
        }
        return;
      }
    }

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
          if (exceedsLimit) {
            break;
          }
          state->currentPhase = PHASE_END;
          state->isPlayerTurn = false;
          state->totalTurnCount++;
          state->normalSummonsThisTurn = 0;
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
        if (exceedsLimit) {
          break;
        }
        state->currentPhase = PHASE_END;
        state->isPlayerTurn = false;
        state->totalTurnCount++;
        state->normalSummonsThisTurn = 0;
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
          state->selectedAttacker = NULL;
          clickedHand = true;
          break;
        }
      }

      if (!clickedHand && state->selectedCardIndexInHand >= 0) {
        int maxSummons = (state->currentPhase == PHASE_PREPARATION) ? 2 : 1;
        if (state->normalSummonsThisTurn >= maxSummons) {
          state->selectedCardIndexInHand = -1;
        } else {
          const Card *cardToSummon = state->playerHand[state->selectedCardIndexInHand];
          int reqTributes = 0;
          if (cardToSummon->stars >= 5) reqTributes = 1;

          bool placed = false;

          for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect)) {
              if (reqTributes > 0) {
                if (!state->playerDefRow[i].isEmpty && state->playerDefRow[i].card->stars <= 4) {
                  state->playerDefRow[i].isEmpty = true;
                  PlaceCardInSlot(state, &state->playerDefRow[i], false);
                  state->normalSummonsThisTurn++;
                  placed = true;
                }
              } else {
                if (state->playerDefRow[i].isEmpty) {
                  PlaceCardInSlot(state, &state->playerDefRow[i], false);
                  state->normalSummonsThisTurn++;
                  placed = true;
                }
              }
              break;
            }

            if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect)) {
              if (reqTributes > 0) {
                if (!state->playerAtkRow[i].isEmpty && state->playerAtkRow[i].card->stars <= 4) {
                  state->playerAtkRow[i].isEmpty = true;
                  PlaceCardInSlot(state, &state->playerAtkRow[i], true);
                  state->normalSummonsThisTurn++;
                  placed = true;
                }
              } else {
                if (state->playerAtkRow[i].isEmpty) {
                  PlaceCardInSlot(state, &state->playerAtkRow[i], true);
                  state->normalSummonsThisTurn++;
                  placed = true;
                }
              }
              break;
            }
          }
          // if (!placed) state->selectedCardIndexInHand = -1; // Removed to prevent accidental deselection
        }
      } else if (!clickedHand && state->selectedCardIndexInHand == -1) {
        if (state->selectedAttacker == NULL) {
          for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect) && !state->playerDefRow[i].isEmpty && !state->playerDefRow[i].positionChangedThisTurn && !state->playerDefRow[i].hasAttacked) {
              state->selectedAttacker = &state->playerDefRow[i];
              break;
            }
            if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect) && !state->playerAtkRow[i].isEmpty && !state->playerAtkRow[i].positionChangedThisTurn && !state->playerAtkRow[i].hasAttacked) {
              state->selectedAttacker = &state->playerAtkRow[i];
              break;
            }
          }
        } else {
          for (int i = 0; i < 5; i++) {
            if (CheckCollisionPointRec(mousePos, state->playerDefRow[i].rect) && state->playerDefRow[i].isEmpty) {
              state->playerDefRow[i].card = state->selectedAttacker->card;
              state->playerDefRow[i].isDefending = false;
              state->playerDefRow[i].isEmpty = false;
              state->playerDefRow[i].hasAttacked = true;
              state->playerDefRow[i].positionChangedThisTurn = true;
              state->playerDefRow[i].summonedThisTurn = state->selectedAttacker->summonedThisTurn;

              state->selectedAttacker->isEmpty = true;
              state->selectedAttacker->card = NULL;
              break;
            }
            if (CheckCollisionPointRec(mousePos, state->playerAtkRow[i].rect) && state->playerAtkRow[i].isEmpty) {
              state->playerAtkRow[i].card = state->selectedAttacker->card;
              state->playerAtkRow[i].isDefending = true;
              state->playerAtkRow[i].isEmpty = false;
              state->playerAtkRow[i].hasAttacked = true;
              state->playerAtkRow[i].positionChangedThisTurn = true;
              state->playerAtkRow[i].summonedThisTurn = state->selectedAttacker->summonedThisTurn;

              state->selectedAttacker->isEmpty = true;
              state->selectedAttacker->card = NULL;
              break;
            }
          }
          state->selectedAttacker = NULL;
        }
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