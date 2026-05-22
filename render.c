#include "render.h"
#include "game_data.h"
#include <stdio.h>
#include <string.h>

#define LEFT_PANEL_WIDTH 300
#define RIGHT_PANEL_WIDTH 980
#define CARD_WIDTH 100
#define CARD_HEIGHT 140
#define SLOT_GAP 10

void InitBattlefieldLayout(GameState *state, int screenWidth, int screenHeight) {
    int rightPanelStartX = LEFT_PANEL_WIDTH;
    int rightPanelCenterX = rightPanelStartX + (RIGHT_PANEL_WIDTH / 2);
    int centerY = screenHeight / 2;
    
    int rowWidth = 5 * CARD_WIDTH + 4 * SLOT_GAP;
    int startX = rightPanelCenterX - rowWidth / 2;
    
    // ĐÃ ĐẢO NGƯỢC VỊ TRÍ HÀNG CÔNG & THỦ
    int enemyAtkY = centerY - 2 * CARD_HEIGHT - 30; // Địch Công: Phía sau
    int enemyDefY = centerY - CARD_HEIGHT - 10;     // Địch Thủ: Phía trước
    int playerDefY = centerY + 10;                  // Ta Thủ: Phía trước
    int playerAtkY = centerY + CARD_HEIGHT + 30;    // Ta Công: Phía sau

    for(int i=0; i<5; i++) {
        int x = startX + i * (CARD_WIDTH + SLOT_GAP);
        
        state->enemyDefRow[i].rect = (Rectangle){x, enemyDefY, CARD_WIDTH, CARD_HEIGHT};
        state->enemyAtkRow[i].rect = (Rectangle){x, enemyAtkY, CARD_WIDTH, CARD_HEIGHT};
        state->playerAtkRow[i].rect = (Rectangle){x, playerAtkY, CARD_WIDTH, CARD_HEIGHT};
        state->playerDefRow[i].rect = (Rectangle){x, playerDefY, CARD_WIDTH, CARD_HEIGHT};
        
        state->enemyDefRow[i].isEmpty = true;
        state->enemyAtkRow[i].isEmpty = true;
        state->playerAtkRow[i].isEmpty = true;
        state->playerDefRow[i].isEmpty = true;
        
        state->enemyDefRow[i].isDefending = true;
        state->playerDefRow[i].isDefending = true;
        state->enemyAtkRow[i].isDefending = false;
        state->playerAtkRow[i].isDefending = false;

        state->enemyDefRow[i].hasAttacked = false;
        state->enemyAtkRow[i].hasAttacked = false;
        state->playerAtkRow[i].hasAttacked = false;
        state->playerDefRow[i].hasAttacked = false;
    }

    state->btnNextRect = (Rectangle){25, GetScreenHeight() - 110, 250, 40};
    state->btnExitRect = (Rectangle){25, GetScreenHeight() - 60, 250, 40};
}

static void DrawSlot(GameState *state, Slot *slot, Color outlineColor) {
    DrawRectangleLinesEx(slot->rect, 1, Fade(outlineColor, 0.5f));
    
    if(!slot->isEmpty && slot->card != NULL) {
        if(slot->card->texture.id > 0) {
            Rectangle source = {0, 0, slot->card->texture.width, slot->card->texture.height};
            if (slot->isDefending) {
                Vector2 origin = { CARD_HEIGHT/2, CARD_WIDTH/2 };
                Rectangle dest = { slot->rect.x + CARD_WIDTH/2, slot->rect.y + CARD_HEIGHT/2, CARD_HEIGHT, CARD_WIDTH };
                DrawTexturePro(slot->card->texture, source, dest, origin, 90.0f, WHITE);
            } else {
                DrawTexturePro(slot->card->texture, source, slot->rect, (Vector2){0,0}, 0.0f, WHITE);
            }
        } else {
            DrawRectangleRec(slot->rect, LIGHTGRAY);
            // Đã thu nhỏ font tên tướng từ 10 xuống 8
            DrawText(isEnglishMode ? slot->card->name_en : slot->card->name_vn, slot->rect.x + 2, slot->rect.y + 5, 8, BLACK);
        }
        
        Color highlight = DARKBLUE;
        if (state->selectedAttacker == slot) highlight = RED;
        else if (slot->hasAttacked) highlight = GRAY;

        if (slot->isDefending) {
            Rectangle rotatedRect = {slot->rect.x - (CARD_HEIGHT-CARD_WIDTH)/2, slot->rect.y + (CARD_HEIGHT-CARD_WIDTH)/2, CARD_HEIGHT, CARD_WIDTH};
            DrawRectangleLinesEx(rotatedRect, 3, highlight);
        } else {
            DrawRectangleLinesEx(slot->rect, 3, highlight);
        }
        
        char stats[32];
        sprintf(stats, "%d/%d", slot->card->atk, slot->card->def);
        int textY = slot->isDefending ? slot->rect.y + CARD_HEIGHT/2 - 7 : slot->rect.y + CARD_HEIGHT - 20;
        // Đã thu nhỏ font chỉ số từ 14 xuống 12
        DrawText(stats, slot->rect.x + 2, textY, 12, YELLOW);
    }
}

static void DrawLeftPanel(GameState *state) {
    Rectangle panelRect = {0, 0, LEFT_PANEL_WIDTH, GetScreenHeight()};
    DrawRectangleRec(panelRect, (Color){30, 41, 59, 255});
    
    DrawText(isEnglishMode ? "CARD INFO" : "THONG TIN THE", 60, 20, 24, GOLD);

    const Card* displayCard = state->hoveredCard;
    if (!displayCard && state->selectedCardIndexInHand >= 0) {
        displayCard = state->playerHand[state->selectedCardIndexInHand];
    }
    if (!displayCard && state->selectedAttacker != NULL) {
        displayCard = state->selectedAttacker->card;
    }

    int imgX = 25;
    int imgY = 60;
    int imgW = 250;
    int imgH = 350;

    if (displayCard) {
        if(displayCard->texture.id > 0) {
            Rectangle source = {0, 0, displayCard->texture.width, displayCard->texture.height};
            Rectangle dest = {imgX, imgY, imgW, imgH};
            DrawTexturePro(displayCard->texture, source, dest, (Vector2){0,0}, 0.0f, WHITE);
            DrawRectangleLinesEx(dest, 3, GOLD);
        }
        
        Rectangle infoRect = {imgX, imgY + imgH + 10, imgW, 150};
        DrawRectangleRec(infoRect, RAYWHITE);
        
        DrawText(isEnglishMode ? displayCard->name_en : displayCard->name_vn, infoRect.x + 10, infoRect.y + 10, 20, BLACK);
        
        char starsStr[16] = "";
        for (int s=0; s<displayCard->stars; s++) strcat(starsStr, "*");
        DrawText(starsStr, infoRect.x + 10, infoRect.y + 35, 20, ORANGE);
        
        char statsStr[64];
        sprintf(statsStr, "ATK: %d / DEF: %d", displayCard->atk, displayCard->def);
        DrawText(statsStr, infoRect.x + 10, infoRect.y + 60, 16, DARKBLUE);
        
        DrawText(isEnglishMode ? "Details:" : "Chi tiet:", infoRect.x + 10, infoRect.y + 90, 14, GRAY);
        char shortDesc[100];
        strncpy(shortDesc, isEnglishMode ? displayCard->desc_en : displayCard->desc_vn, 99);
        shortDesc[99] = '\0';
        DrawText(shortDesc, infoRect.x + 10, infoRect.y + 110, 12, DARKGRAY);

    } else {
        if(cardBackTexture.id > 0) {
            Rectangle source = {0, 0, cardBackTexture.width, cardBackTexture.height};
            Rectangle dest = {imgX, imgY, imgW, imgH};
            DrawTexturePro(cardBackTexture, source, dest, (Vector2){0,0}, 0.0f, WHITE);
        }
        
        Rectangle infoRect = {imgX, imgY + imgH + 10, imgW, 150};
        DrawRectangleRec(infoRect, RAYWHITE);
        DrawText(isEnglishMode ? "No card selected" : "Chua chon the bai", infoRect.x + 10, infoRect.y + 10, 20, GRAY);
    }

    DrawRectangleRec(state->btnNextRect, GOLD);
    DrawText(isEnglishMode ? "Next Phase" : "Tiep Theo", state->btnNextRect.x + 80, state->btnNextRect.y + 10, 20, BLACK);

    DrawRectangleRec(state->btnExitRect, MAROON);
    DrawText(isEnglishMode ? "EXIT" : "THOAT TRAN", state->btnExitRect.x + 60, state->btnExitRect.y + 10, 20, WHITE);
}

static void DrawRightPanel(GameState *state) {
    if (bgTexture.id > 0) {
        Rectangle source = {0, 0, bgTexture.width, bgTexture.height};
        Rectangle dest = {LEFT_PANEL_WIDTH, 0, RIGHT_PANEL_WIDTH, GetScreenHeight()};
        DrawTexturePro(bgTexture, source, dest, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawRectangle(LEFT_PANEL_WIDTH, 0, RIGHT_PANEL_WIDTH, GetScreenHeight(), BEIGE);
    }

    DrawText(TextFormat("P1: %d", state->playerLP), LEFT_PANEL_WIDTH + 50, 30, 30, GREEN);
    DrawText(TextFormat("P2: %d", state->enemyLP), GetScreenWidth() - 180, 30, 30, RED);

    const char* phaseStr = isEnglishMode ? "MAIN PHASE" : "GIAI DOAN CHINH";
    if (state->currentPhase == PHASE_BATTLE) phaseStr = isEnglishMode ? "BATTLE PHASE" : "GIAI DOAN CHIEN DAU";
    else if (state->currentPhase == PHASE_END) phaseStr = isEnglishMode ? "ENEMY TURN" : "LUOT KE DICH";
    
    DrawText(phaseStr, LEFT_PANEL_WIDTH + RIGHT_PANEL_WIDTH/2 - MeasureText(phaseStr, 24)/2, 40, 24, GOLD);

    for(int i=0; i<5; i++) {
        DrawSlot(state, &state->enemyDefRow[i], WHITE);
        DrawSlot(state, &state->enemyAtkRow[i], WHITE);
        DrawSlot(state, &state->playerAtkRow[i], WHITE);
        DrawSlot(state, &state->playerDefRow[i], WHITE);
    }
    
    int handStartX = LEFT_PANEL_WIDTH + (RIGHT_PANEL_WIDTH - (state->playerHandCount * (CARD_WIDTH + 10))) / 2;
    int handY = GetScreenHeight() - CARD_HEIGHT - 20;
    
    for(int i=0; i<state->playerHandCount; i++) {
        state->playerHandRects[i] = (Rectangle){handStartX + i * (CARD_WIDTH + 10), handY, CARD_WIDTH, CARD_HEIGHT};
        const Card* c = state->playerHand[i];
        bool isSelected = (state->selectedCardIndexInHand == i);
        
        if(c->texture.id > 0) {
            Rectangle source = {0, 0, c->texture.width, c->texture.height};
            DrawTexturePro(c->texture, source, state->playerHandRects[i], (Vector2){0,0}, 0.0f, WHITE);
        } else {
            DrawRectangleRec(state->playerHandRects[i], LIGHTGRAY);
            DrawText(isEnglishMode ? c->name_en : c->name_vn, state->playerHandRects[i].x + 5, state->playerHandRects[i].y + 10, 10, BLACK);
        }
        
        if (isSelected) {
            DrawRectangleLinesEx(state->playerHandRects[i], 4, GOLD);
        } else {
            DrawRectangleLinesEx(state->playerHandRects[i], 2, DARKGRAY);
        }
    }

    int enemyHandY = 80;
    for(int i=0; i<5; i++) {
        Rectangle dest = {handStartX + i * (CARD_WIDTH + 10), enemyHandY, CARD_WIDTH, CARD_HEIGHT};
        if(cardBackTexture.id > 0) {
            Rectangle source = {0, 0, cardBackTexture.width, cardBackTexture.height};
            DrawTexturePro(cardBackTexture, source, dest, (Vector2){0,0}, 0.0f, WHITE);
        } else {
            DrawRectangleRec(dest, DARKGRAY);
        }
        DrawRectangleLinesEx(dest, 1, GRAY);
    }

    int deckX = GetScreenWidth() - CARD_WIDTH - 30;
    Rectangle playerDeckRect = {deckX, GetScreenHeight() - CARD_HEIGHT - 20, CARD_WIDTH, CARD_HEIGHT};
    Rectangle enemyDeckRect = {deckX, enemyHandY, CARD_WIDTH, CARD_HEIGHT};

    if(cardBackTexture.id > 0) {
        Rectangle source = {0, 0, cardBackTexture.width, cardBackTexture.height};
        DrawTexturePro(cardBackTexture, source, playerDeckRect, (Vector2){0,0}, 0.0f, WHITE);
        DrawTexturePro(cardBackTexture, source, enemyDeckRect, (Vector2){0,0}, 0.0f, WHITE);
    }
    DrawRectangleLinesEx(playerDeckRect, 2, GOLD);
    DrawRectangleLinesEx(enemyDeckRect, 2, RED);
    
    DrawRectangleRec((Rectangle){playerDeckRect.x, playerDeckRect.y + CARD_HEIGHT, CARD_WIDTH, 20}, BLACK);
    DrawText(isEnglishMode ? "DECK" : "XAP BAN", playerDeckRect.x + 20, playerDeckRect.y + CARD_HEIGHT + 3, 12, GOLD);

    DrawRectangleRec((Rectangle){enemyDeckRect.x, enemyDeckRect.y + CARD_HEIGHT, CARD_WIDTH, 20}, BLACK);
    DrawText(isEnglishMode ? "ENEMY" : "XAP DICH", enemyDeckRect.x + 15, enemyDeckRect.y + CARD_HEIGHT + 3, 12, RED);
}

void DrawBattlefield(GameState *state) {
    DrawRightPanel(state);
    DrawLeftPanel(state);
}