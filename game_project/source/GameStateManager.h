#pragma once

typedef void(*FP)(void);

extern int current, previous, next;

extern  FP fpLoad, fpInitialize, fpDraw, fpFree, fpUnload;

extern void(*fpUpdate)(float dt);

void GSM_Initialize(int startingState);
//void GSM_Update();
void GSM_Update(float dt);
void GSM_Draw();