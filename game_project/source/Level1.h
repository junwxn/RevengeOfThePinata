#pragma once

typedef struct Circle
{
	f32 pos_x, pos_y;
	f32 r;
}Circle;

typedef struct RectData {
	f32 pos_x, pos_y, w, h;
	f32 max, min, current, var;
}RectData;

void Level1_Load();
void Level1_Init();
void Level1_Update(float dt);
void Level1_Draw();
void Level1_Free();
void Level1_Unload();

void SpawnWave1();
void SpawnWave2();