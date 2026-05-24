#include "render.h"
#include "game_data.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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
    
    int enemyDefY = fieldStartY; 
    int enemyAtkY = fieldStartY + cardH + 8;     
    int playerAtkY = fieldStartY + 2 * (cardH + 8);                  
    int playerDefY = fieldStartY + 3 * (cardH + 8);    

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
        int infoBoxY = imgY + imgH + 10;
        int infoBoxH = screenH - (imgY + imgH + 150);
        DrawRectangleRec((Rectangle){imgX, infoBoxY, imgW, infoBoxH}, RAYWHITE);
        
        // Tên tướng
        DrawText(isEnglishMode ? displayCard->name_en : displayCard->name_vn, imgX + 10, infoBoxY + 10, 20, BLACK);
        
        // === HIỂN THỊ ATK/DEF ===
        int statsY = infoBoxY + 38;
        
        // ATK
        char atkStr[32];
        sprintf(atkStr, "ATK: %d", displayCard->atk);
        DrawText(atkStr, imgX + 10, statsY, 18, (Color){200, 50, 50, 255});
        
        // DEF  
        char defStr[32];
        sprintf(defStr, "DEF: %d", displayCard->def);
        int atkTextW = MeasureText(atkStr, 18);
        DrawText(defStr, imgX + 10 + atkTextW + 20, statsY, 18, (Color){50, 100, 200, 255});
        
        // Loại tướng + Số sao
        int typeY = statsY + 24;
        const char* typeStr = displayCard->type == TYPE_VO ? (isEnglishMode ? "Warrior" : "Vo Tuong") : (isEnglishMode ? "Strategist" : "Van Tuong");
        char starStr[64];
        sprintf(starStr, "%s | ", typeStr);
        // Vẽ sao
        int starTextX = imgX + 10;
        DrawText(starStr, starTextX, typeY, 16, (Color){120, 120, 120, 255});
        starTextX += MeasureText(starStr, 16);
        for (int s = 0; s < displayCard->stars; s++) {
            DrawText("*", starTextX + s * 14, typeY, 16, GOLD);
        }
        
        // Đường kẻ phân cách
        int sepY = typeY + 24;
        DrawRectangle(imgX + 10, sepY, imgW - 20, 1, (Color){200, 200, 200, 255});
        
        // Mô tả
        DrawTextWrapped(isEnglishMode ? displayCard->desc_en : displayCard->desc_vn, imgX + 10, sepY + 8, 16, DARKGRAY, imgW - 20);
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
        case PHASE_PREPARATION: phaseStr = isEnglishMode ? "PREPARATION" : "CHUAN BI"; break;
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
    
    // === VẼ DECK PLAYER Ở MÉP PHẢI ===
    int deckCardW = handCardW * 0.7;
    int deckCardH = deckCardW * 1.4;
    int deckX = screenW - deckCardW - 15;
    int deckY = handY + (handCardH - deckCardH) / 2;
    
    if (state->playerDeckCount > 0) {
        // Vẽ hiệu ứng chồng bài (3 lớp)
        int stackLayers = state->playerDeckCount > 3 ? 3 : state->playerDeckCount;
        for (int layer = stackLayers - 1; layer >= 0; layer--) {
            int offsetX = layer * 2;
            int offsetY = -layer * 2;
            Color tint = (layer == 0) ? WHITE : (Color){200, 200, 200, 200};
            if (cardBackTexture.id > 0) {
                DrawTexturePro(cardBackTexture, 
                    (Rectangle){0, 0, cardBackTexture.width, cardBackTexture.height},
                    (Rectangle){deckX + offsetX, deckY + offsetY, deckCardW, deckCardH},
                    (Vector2){0,0}, 0.0f, tint);
            } else {
                DrawRectangle(deckX + offsetX, deckY + offsetY, deckCardW, deckCardH, (Color){100, 50, 20, 255});
            }
            if (layer == 0) {
                DrawRectangleLinesEx((Rectangle){deckX, deckY, deckCardW, deckCardH}, 2, GOLD);
            }
        }
    } else {
        // Deck trống - vẽ viền mờ
        DrawRectangleLinesEx((Rectangle){deckX, deckY, deckCardW, deckCardH}, 2, Fade(GOLD, 0.3f));
    }
    
    // Hiển thị số bài còn lại trong deck
    char deckCountStr[16];
    sprintf(deckCountStr, "DECK:%d", state->playerDeckCount);
    DrawText(deckCountStr, deckX - 5, deckY + deckCardH + 3, 11, GOLD);
    
    // === VẼ DECK ENEMY Ở GÓC TRÊN PHẢI (dưới thanh LP) ===
    int enemyDeckX = screenW - deckCardW - 15;
    int enemyDeckY = 40;
    int enemyDeckH = deckCardH * 0.6;
    int enemyDeckW = enemyDeckH / 1.4;
    
    if (state->enemyDeckCount > 0) {
        int eLayers = state->enemyDeckCount > 3 ? 3 : state->enemyDeckCount;
        for (int layer = eLayers - 1; layer >= 0; layer--) {
            Color tint = (layer == 0) ? WHITE : (Color){200, 200, 200, 200};
            if (cardBackTexture.id > 0) {
                DrawTexturePro(cardBackTexture, 
                    (Rectangle){0, 0, cardBackTexture.width, cardBackTexture.height},
                    (Rectangle){enemyDeckX + layer*1, enemyDeckY - layer*1, enemyDeckW, enemyDeckH},
                    (Vector2){0,0}, 0.0f, tint);
            } else {
                DrawRectangle(enemyDeckX + layer*1, enemyDeckY - layer*1, enemyDeckW, enemyDeckH, (Color){100, 50, 20, 255});
            }
        }
        DrawRectangleLinesEx((Rectangle){enemyDeckX, enemyDeckY, enemyDeckW, enemyDeckH}, 1, (Color){255, 80, 80, 200});
    }
    char enemyDeckStr[16];
    sprintf(enemyDeckStr, "DECK:%d", state->enemyDeckCount);
    DrawText(enemyDeckStr, enemyDeckX - 5, enemyDeckY + enemyDeckH + 3, 11, (Color){255, 80, 80, 200});
}

// === VẼ HIỆU ỨNG RÚT BÀI ===
static void DrawCardDrawAnimation(GameState *state) {
    if (!state->drawAnim.active || !state->drawAnim.card) return;
    
    float t = state->drawAnim.timer / state->drawAnim.duration;
    if (t > 1.0f) t = 1.0f;
    
    // Easing: ease-out cubic cho chuyển động mượt
    float ease = 1.0f - powf(1.0f - t, 3.0f);
    
    // Nội suy vị trí
    float curX = state->drawAnim.startPos.x + (state->drawAnim.endPos.x - state->drawAnim.startPos.x) * ease;
    float curY = state->drawAnim.startPos.y + (state->drawAnim.endPos.y - state->drawAnim.startPos.y) * ease;
    
    // Nội suy kích thước
    float curW = state->drawAnim.startW + (state->drawAnim.endW - state->drawAnim.startW) * ease;
    float curH = state->drawAnim.startH + (state->drawAnim.endH - state->drawAnim.startH) * ease;
    
    // Hiệu ứng nâng lên (arc) - bài bay lên cao rồi hạ xuống
    float arcHeight = -80.0f * sinf(t * 3.14159f);
    curY += arcHeight;
    
    // === VẼ GLOW EFFECT ===
    float glowAlpha = 0.4f * sinf(t * 3.14159f); // Glow mạnh nhất ở giữa animation
    Color glowColor = (Color){255, 215, 0, (unsigned char)(glowAlpha * 255)};
    DrawRectangle(curX - 6, curY - 6, curW + 12, curH + 12, glowColor);
    DrawRectangle(curX - 3, curY - 3, curW + 6, curH + 6, (Color){255, 240, 150, (unsigned char)(glowAlpha * 200)});
    
    // === HIỆU ỨNG LẬT BÀI ===
    float flipProgress = state->drawAnim.flipTimer / (state->drawAnim.duration * 0.4f);
    if (flipProgress > 1.0f) flipProgress = 1.0f;
    
    // Scale X để tạo hiệu ứng lật (thu nhỏ -> mở rộng)
    float scaleX = 1.0f;
    if (!state->drawAnim.showFront) {
        // Đang thu nhỏ chiều ngang (chuẩn bị lật)
        scaleX = 1.0f - flipProgress;
        if (scaleX < 0.05f) scaleX = 0.05f;
    } else {
        // Đã lật, mở rộng lại
        float flipExpandT = (state->drawAnim.flipTimer - state->drawAnim.duration * 0.4f) / (state->drawAnim.duration * 0.2f);
        if (flipExpandT < 0.0f) flipExpandT = 0.0f;
        if (flipExpandT > 1.0f) flipExpandT = 1.0f;
        scaleX = flipExpandT;
        if (scaleX < 0.05f) scaleX = 0.05f;
    }
    
    float drawW = curW * scaleX;
    float drawX = curX + (curW - drawW) / 2.0f; // Căn giữa khi thu nhỏ
    
    Rectangle dest = {drawX, curY, drawW, curH};
    
    if (state->drawAnim.showFront && state->drawAnim.card->texture.id > 0) {
        // Vẽ mặt trước
        Rectangle src = {0, 0, state->drawAnim.card->texture.width, state->drawAnim.card->texture.height};
        DrawTexturePro(state->drawAnim.card->texture, src, dest, (Vector2){0,0}, 0.0f, WHITE);
        DrawRectangleLinesEx(dest, 3, GOLD);
    } else {
        // Vẽ mặt sau
        if (cardBackTexture.id > 0) {
            Rectangle src = {0, 0, cardBackTexture.width, cardBackTexture.height};
            DrawTexturePro(cardBackTexture, src, dest, (Vector2){0,0}, 0.0f, WHITE);
        } else {
            DrawRectangleRec(dest, (Color){100, 50, 20, 255});
        }
        DrawRectangleLinesEx(dest, 2, (Color){180, 150, 80, 255});
    }
    
    // Vẽ particles/sparkle nhỏ xung quanh
    float particleTime = state->drawAnim.timer * 8.0f;
    for (int p = 0; p < 6; p++) {
        float angle = particleTime + p * 1.047f; // 60 độ mỗi particle
        float radius = 15.0f + 10.0f * sinf(particleTime + p);
        float px = curX + curW/2 + cosf(angle) * radius;
        float py = curY + curH/2 + sinf(angle) * radius;
        float pAlpha = 0.6f * sinf(t * 3.14159f);
        DrawCircle(px, py, 2.5f, (Color){255, 230, 100, (unsigned char)(pAlpha * 255)});
    }
}

void DrawBattlefield(GameState *state) {
    DrawRightPanel(state);
    DrawLeftPanel(state);
    DrawCardDrawAnimation(state);

    if (state->isRollingDice) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        
        // Vẽ overlay làm mờ nền
        DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 0, 150});
        
        // Vẽ box chứa thông tin đổ xúc xắc
        int boxW = 500;
        int boxH = 300;
        int boxX = (screenW - boxW) / 2;
        int boxY = (screenH - boxH) / 2;
        
        DrawRectangle(boxX, boxY, boxW, boxH, DARKBLUE);
        DrawRectangleLinesEx((Rectangle){boxX, boxY, boxW, boxH}, 4, GOLD);
        
        const char* titleStr = isEnglishMode ? "ROLLING DICE TO DECIDE TURN!" : "DO XUC XAC GIANH QUYEN DI TRUOC!";
        DrawText(titleStr, boxX + boxW/2 - MeasureText(titleStr, 24)/2, boxY + 30, 24, WHITE);
        
        // Xúc xắc Player (Trái)
        DrawRectangle(boxX + 100, boxY + 120, 80, 80, RAYWHITE);
        DrawRectangleLinesEx((Rectangle){boxX + 100, boxY + 120, 80, 80}, 2, BLACK);
        char pDiceStr[4];
        sprintf(pDiceStr, "%d", state->playerDiceValue);
        DrawText(pDiceStr, boxX + 100 + 40 - MeasureText(pDiceStr, 40)/2, boxY + 120 + 20, 40, BLACK);
        DrawText("P1", boxX + 100 + 40 - MeasureText("P1", 20)/2, boxY + 220, 20, GREEN);
        
        // Xúc xắc Enemy (Phải)
        DrawRectangle(boxX + 320, boxY + 120, 80, 80, RAYWHITE);
        DrawRectangleLinesEx((Rectangle){boxX + 320, boxY + 120, 80, 80}, 2, BLACK);
        char eDiceStr[4];
        sprintf(eDiceStr, "%d", state->enemyDiceValue);
        DrawText(eDiceStr, boxX + 320 + 40 - MeasureText(eDiceStr, 40)/2, boxY + 120 + 20, 40, BLACK);
        DrawText("P2", boxX + 320 + 40 - MeasureText("P2", 20)/2, boxY + 220, 20, RED);
        
        // Chữ VS ở giữa
        DrawText("VS", boxX + boxW/2 - MeasureText("VS", 30)/2, boxY + 145, 30, GOLD);
    }
}