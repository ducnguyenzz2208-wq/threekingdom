#include "render.h"
#include "game_data.h"
#include <stdio.h>
#include <string.h>

// Hàm helper để vẽ chữ tự động xuống dòng (Word Wrap)
static void DrawTextWrapped(const char* text, int x, int y, int fontSize, Color color, int maxWidth) {
    char buffer[1024];
    strncpy(buffer, text, 1023);
    buffer[1023] = '\0';
    char *word = strtok(buffer, " ");
    char line[256] = "";
    int currentY = y;
    while (word != NULL) {
        char testLine[256];
        strcpy(testLine, line);
        if (strlen(testLine) > 0) strcat(testLine, " ");
        strcat(testLine, word);
        if (MeasureText(testLine, fontSize) > maxWidth) {
            DrawText(line, x, currentY, fontSize, color);
            strcpy(line, word);
            currentY += fontSize + 5;
        } else {
            strcpy(line, testLine);
        }
        word = strtok(NULL, " ");
    }
    if (strlen(line) > 0) DrawText(line, x, currentY, fontSize, color);
}

void InitBattlefieldLayout(GameState *state, int screenWidth, int screenHeight) {
    int leftPanelW = screenWidth * 0.25;
    if (leftPanelW < 300) leftPanelW = 300;
    int rightPanelW = screenWidth - leftPanelW;
    
    // === PHÂN CHIA VÙNG ===
    // Vùng trên: LP + bài úp đối thủ (khoảng 8%)
    // Vùng giữa: Sân đấu 4 hàng (khoảng 65%)
    // Vùng dưới: Tay bài người chơi (khoảng 22%)
    // Khoảng trống giữa các vùng (5%)
    
    int topBarH = screenHeight * 0.06;       // Vùng hiển thị LP + phase
    int handAreaH = screenHeight * 0.22;     // Vùng tay bài ở dưới cùng
    int fieldTopY = topBarH + 10;            // Sân đấu bắt đầu sau top bar
    int fieldBottomY = screenHeight - handAreaH - 10; // Sân đấu kết thúc trước hand area
    int fieldH = fieldBottomY - fieldTopY;   // Chiều cao vùng sân đấu
    
    // Tính kích thước bài trên sân
    int playAreaW = rightPanelW * 0.8; 
    int cardH = (fieldH - 50) / 4;           // 4 hàng với khoảng cách
    int cardW = cardH / 1.4;
    
    // Giới hạn chiều rộng bài nếu quá lớn
    int maxCardW = playAreaW / 6;
    if (cardW > maxCardW) {
        cardW = maxCardW;
        cardH = cardW * 1.4;
    }
    
    int gap = cardW * 0.1;
    
    // Căn giữa sân đấu theo chiều ngang
    int playAreaStartX = leftPanelW + (rightPanelW - playAreaW) / 2;
    int rowWidth = 5 * cardW + 4 * gap;
    int startX = playAreaStartX + (playAreaW - rowWidth) / 2;
    
    // Căn giữa 4 hàng trong vùng sân đấu
    int totalFieldContentH = 4 * cardH + 3 * 8; // 4 hàng + 3 khoảng cách 8px
    int fieldStartY = fieldTopY + (fieldH - totalFieldContentH) / 2;
    
    int enemyAtkY = fieldStartY; 
    int enemyDefY = fieldStartY + cardH + 8;     
    int playerDefY = fieldStartY + 2 * (cardH + 8);                  
    int playerAtkY = fieldStartY + 3 * (cardH + 8);    

    for(int i=0; i<5; i++) {
        int x = startX + i * (cardW + gap);
        state->enemyAtkRow[i].rect = (Rectangle){x, enemyAtkY, cardW, cardH};
        state->enemyDefRow[i].rect = (Rectangle){x, enemyDefY, cardW, cardH};
        state->playerDefRow[i].rect = (Rectangle){x, playerDefY, cardW, cardH};
        state->playerAtkRow[i].rect = (Rectangle){x, playerAtkY, cardW, cardH};
    }
    
    // === TÍNH VỊ TRÍ TAY BÀI ===
    int handY = screenHeight - handAreaH + 10;  // Vị trí Y của tay bài
    int handCardH = handAreaH - 25;             // Chiều cao bài trên tay
    int handCardW = handCardH / 1.4;
    int handGap = 8;
    
    // Căn giữa tay bài trong vùng bên phải
    int totalHandW = state->playerHandCount * handCardW + (state->playerHandCount - 1) * handGap;
    int handStartX = leftPanelW + (rightPanelW - totalHandW) / 2;
    
    for (int i = 0; i < state->playerHandCount; i++) {
        state->playerHandRects[i] = (Rectangle){
            handStartX + i * (handCardW + handGap),
            handY,
            handCardW,
            handCardH
        };
    }

    state->btnNextRect = (Rectangle){25, screenHeight - 110, leftPanelW - 50, 40};
    state->btnExitRect = (Rectangle){25, screenHeight - 60, leftPanelW - 50, 40};
}

static void DrawSlot(GameState *state, Slot *slot, Color outlineColor) {
    DrawRectangleLinesEx(slot->rect, 2, Fade(outlineColor, 0.5f));
    float cW = slot->rect.width;
    float cH = slot->rect.height;

    if(!slot->isEmpty && slot->card != NULL) {
        if(slot->card->texture.id > 0) {
            Rectangle source = {0, 0, slot->card->texture.width, slot->card->texture.height};
            if (slot->isDefending) {
                Vector2 origin = { cH/2, cW/2 };
                Rectangle dest = { slot->rect.x + cW/2, slot->rect.y + cH/2, cH, cW };
                DrawTexturePro(slot->card->texture, source, dest, origin, 90.0f, WHITE);
            } else {
                DrawTexturePro(slot->card->texture, source, slot->rect, (Vector2){0,0}, 0.0f, WHITE);
            }
        }
        
        Color highlight = DARKBLUE;
        if (state->selectedAttacker == slot) highlight = RED;
        else if (slot->hasAttacked) highlight = GRAY;

        if (slot->isDefending) {
            Rectangle rotatedRect = {slot->rect.x - (cH-cW)/2, slot->rect.y + (cH-cW)/2, cH, cW};
            DrawRectangleLinesEx(rotatedRect, 4, highlight);
        } else {
            DrawRectangleLinesEx(slot->rect, 4, highlight);
        }
        
        char stats[32];
        sprintf(stats, "%d/%d", slot->card->atk, slot->card->def);
        int textY = slot->isDefending ? slot->rect.y + cH/2 - 7 : slot->rect.y + cH - 20;
        DrawText(stats, slot->rect.x + 4, textY, cW * 0.16, YELLOW);
    }
}

static void DrawLeftPanel(GameState *state) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int leftW = screenW * 0.25;
    if (leftW < 300) leftW = 300;
    
    DrawRectangle(0, 0, leftW, screenH, (Color){30, 41, 59, 255});
    DrawText(isEnglishMode ? "CARD INFO" : "THONG TIN THE", leftW/2 - MeasureText("THONG TIN THE", 24)/2, 20, 24, GOLD);

    const Card* displayCard = state->hoveredCard;
    if (!displayCard && state->selectedCardIndexInHand >= 0) displayCard = state->playerHand[state->selectedCardIndexInHand];
    if (!displayCard && state->selectedAttacker != NULL) displayCard = state->selectedAttacker->card;

    int imgW = leftW * 0.75;
    int imgH = imgW * 1.4;
    int imgX = (leftW - imgW) / 2;
    int imgY = 60;

    if (displayCard) {
        if(displayCard->texture.id > 0) {
            DrawTexturePro(displayCard->texture, (Rectangle){0,0,displayCard->texture.width,displayCard->texture.height}, (Rectangle){imgX, imgY, imgW, imgH}, (Vector2){0,0}, 0.0f, WHITE);
            DrawRectangleLinesEx((Rectangle){imgX, imgY, imgW, imgH}, 3, GOLD);
        }
        DrawRectangleRec((Rectangle){imgX, imgY + imgH + 10, imgW, screenH - (imgY + imgH + 150)}, RAYWHITE);
        DrawText(isEnglishMode ? displayCard->name_en : displayCard->name_vn, imgX + 10, imgY + imgH + 20, 20, BLACK);
        DrawTextWrapped(isEnglishMode ? displayCard->desc_en : displayCard->desc_vn, imgX + 10, imgY + imgH + 50, 16, DARKGRAY, imgW - 20);
    }

    DrawRectangleRec(state->btnNextRect, GOLD);
    DrawText(isEnglishMode ? "Next Phase" : "Tiep Theo", state->btnNextRect.x + 10, state->btnNextRect.y + 10, 20, BLACK);
    DrawRectangleRec(state->btnExitRect, MAROON);
    DrawText(isEnglishMode ? "EXIT" : "THOAT", state->btnExitRect.x + 10, state->btnExitRect.y + 10, 20, WHITE);
}

static void DrawRightPanel(GameState *state) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    int leftW = screenW * 0.25;
    if (leftW < 300) leftW = 300;
    int rightW = screenW - leftW;
    int handAreaH = screenH * 0.22;
    
    // === VẼ NỀN GAME (chỉ vùng sân đấu, không bao gồm hand area) ===
    int fieldBottomY = screenH - handAreaH - 10;
    if (bgTexture.id > 0) {
        DrawTexturePro(bgTexture, 
            (Rectangle){0, 0, bgTexture.width, bgTexture.height}, 
            (Rectangle){leftW, 0, rightW, fieldBottomY}, 
            (Vector2){0,0}, 0.0f, WHITE);
    }
    
    // === VẼ NỀN VÙNG TAY BÀI (tách biệt) ===
    DrawRectangle(leftW, fieldBottomY, rightW, screenH - fieldBottomY, (Color){20, 20, 35, 240});
    // Đường phân cách giữa sân đấu và tay bài
    DrawRectangle(leftW, fieldBottomY - 2, rightW, 4, (Color){255, 200, 50, 200});

    // === VẼ LP (Life Points) ===
    char lpStr[64];
    sprintf(lpStr, "P1: %d", state->playerLP);
    DrawText(lpStr, leftW + 20, 10, 24, (Color){50, 200, 255, 255});
    
    sprintf(lpStr, "P2: %d", state->enemyLP);
    int p2TextW = MeasureText(lpStr, 24);
    DrawText(lpStr, screenW - p2TextW - 20, 10, 24, (Color){255, 80, 80, 255});

    // === VẼ PHASE HIỆN TẠI ===
    const char* phaseStr = "";
    switch(state->currentPhase) {
        case PHASE_DRAW: phaseStr = isEnglishMode ? "DRAW PHASE" : "RUT BAI"; break;
        case PHASE_STANDBY: phaseStr = isEnglishMode ? "STANDBY" : "CHO"; break;
        case PHASE_MAIN_1: phaseStr = isEnglishMode ? "MAIN PHASE 1" : "GIAI DOAN CHINH"; break;
        case PHASE_BATTLE: phaseStr = isEnglishMode ? "BATTLE" : "CHIEN DAU"; break;
        case PHASE_MAIN_2: phaseStr = isEnglishMode ? "MAIN PHASE 2" : "GIAI DOAN CHINH 2"; break;
        case PHASE_END: phaseStr = isEnglishMode ? "END" : "KET THUC"; break;
    }
    int phaseTextW = MeasureText(phaseStr, 22);
    DrawText(phaseStr, leftW + rightW/2 - phaseTextW/2, 12, 22, GOLD);

    // === VẼ BÀI TRÊN SÂN ĐẤU ===
    for(int i=0; i<5; i++) {
        DrawSlot(state, &state->enemyAtkRow[i], WHITE);
        DrawSlot(state, &state->enemyDefRow[i], WHITE);
        DrawSlot(state, &state->playerDefRow[i], WHITE);
        DrawSlot(state, &state->playerAtkRow[i], WHITE);
    }
    
    // === VẼ TAY BÀI NGƯỜI CHƠI (vùng dưới, tách biệt) ===
    // Cập nhật lại playerHandRects mỗi frame (vì hand count thay đổi)
    int handY = screenH - handAreaH + 10;
    int handCardH = handAreaH - 25;
    int handCardW = handCardH / 1.4;
    int handGap = 8;
    
    // Nếu quá nhiều bài, thu nhỏ khoảng cách
    int totalHandW = state->playerHandCount * handCardW + (state->playerHandCount > 1 ? (state->playerHandCount - 1) * handGap : 0);
    if (totalHandW > rightW - 100) {
        // Overlap cards nếu quá nhiều
        handGap = (rightW - 100 - handCardW) / (state->playerHandCount > 1 ? state->playerHandCount - 1 : 1) - handCardW;
        if (handGap < -handCardW/2) handGap = -handCardW/2;
        totalHandW = state->playerHandCount * handCardW + (state->playerHandCount > 1 ? (state->playerHandCount - 1) * handGap : 0);
    }
    
    int handStartX = leftW + (rightW - totalHandW) / 2;
    
    for (int i = 0; i < state->playerHandCount; i++) {
        int hx = handStartX + i * (handCardW + handGap);
        state->playerHandRects[i] = (Rectangle){hx, handY, handCardW, handCardH};
        
        const Card* card = state->playerHand[i];
        if (card && card->texture.id > 0) {
            if (i == state->selectedCardIndexInHand) {
                // Nâng bài lên 15px khi được chọn
                state->playerHandRects[i].y -= 15;
                DrawTexturePro(card->texture, 
                    (Rectangle){0, 0, card->texture.width, card->texture.height},
                    (Rectangle){hx, handY - 15, handCardW, handCardH},
                    (Vector2){0,0}, 0.0f, WHITE);
                DrawRectangleLinesEx((Rectangle){hx, handY - 15, handCardW, handCardH}, 3, GOLD);
            } else {
                DrawTexturePro(card->texture, 
                    (Rectangle){0, 0, card->texture.width, card->texture.height},
                    (Rectangle){hx, handY, handCardW, handCardH},
                    (Vector2){0,0}, 0.0f, WHITE);
                DrawRectangleLinesEx((Rectangle){hx, handY, handCardW, handCardH}, 2, (Color){180, 180, 180, 150});
            }
        }
    }
    
    // Chữ "Tay bài" nhỏ ở góc
    const char* handLabel = isEnglishMode ? "HAND" : "TAY BAI";
    DrawText(handLabel, leftW + 10, fieldBottomY + 8, 14, (Color){200, 200, 200, 150});
    
    // === VẼ DECK Ở MÉP PHẢI ===
    int deckCardW = handCardW * 0.7;
    int deckCardH = deckCardW * 1.4;
    int deckX = screenW - deckCardW - 15;
    int deckY = handY + (handCardH - deckCardH) / 2;
    
    if (cardBackTexture.id > 0) {
        DrawTexturePro(cardBackTexture, 
            (Rectangle){0, 0, cardBackTexture.width, cardBackTexture.height},
            (Rectangle){deckX, deckY, deckCardW, deckCardH},
            (Vector2){0,0}, 0.0f, WHITE);
    } else {
        DrawRectangle(deckX, deckY, deckCardW, deckCardH, (Color){100, 50, 20, 255});
    }
    DrawRectangleLinesEx((Rectangle){deckX, deckY, deckCardW, deckCardH}, 2, GOLD);
    DrawText("DECK", deckX + 2, deckY + deckCardH + 3, 11, GOLD);
}

void DrawBattlefield(GameState *state) {
    DrawRightPanel(state);
    DrawLeftPanel(state);
}