#pragma once
#include "AEEngine.h"
#include <cmath>
#include "Sprite.h"

// --- Constants ---
const f32 GRID_W = 111.0f;
const f32 GRID_H = 64.0f;
const f32 SPRITE_W = 111.0f;
const f32 SPRITE_H = 128.0f;

const f32 PLAYER_SIZE = 25.0f;
const f32 ENEMY_SIZE  = 25.0f;
const f32 BOSS_SIZE   = 50.0f;

// --- Structs ---
struct Vec2 {
    f32 x, y;
};

struct Circle {
    f32 pos_x, pos_y;
    f32 r;
};

struct RectData {
    f32 pos_x, pos_y, w, h;
    f32 max, min, current, var;
};

// --- Math Helpers ---
// Converts Grid (Iso) coordinates to Screen (Cartesian) coordinates
Vec2 GridToScreen(int gridX, int gridY);

// Collision check
bool AreCirclesIntersecting(float c1_x, float c1_y, float r1, float c2_x, float c2_y, float r2);

// Collision check for cursor (AABB)
bool IsMouseInside(float mousepos_x, float mousepos_y, float center_x, float center_y, float width, float height);
// mousepos_x, mousepos_y -> cursor position
// center_x, center_y -> center of rectangle
// width, height -> width and height of rectangle
// returns true only if mouse is inside

// --- Graphics Helpers ---
// Creates a mesh and RETURNS the pointer (instead of setting a global)
AEGfxVertexList* CreateCircleMesh(f32 radius, u8 parts, u32 color);
AEGfxVertexList* CreateRectMesh(u32 color);
AEGfxVertexList* CreateTriangleMesh(u32 color);
AEGfxVertexList* CreateLineMesh(float range, u32 color);
AEGfxVertexList* CreateAttackRangeMesh(f32 attackRange, u32 color);
AEGfxVertexList* CreateRingMesh(int segments, f32 thickness);

AEGfxVertexList* CreateSpriteRectMesh(u32 color);

// Generic Draw function that can draw ANY mesh (Circle or Rect)
// This replaces specific DrawCircle/DrawRect functions by letting you pass the mesh you want to draw.
void DrawMesh(AEGfxVertexList* pMesh, float width, float height, float x, float y, float rot, float r, float g, float b, float a);
void DrawTexture(Sprite& spriteObj, int currentDirection, AEGfxVertexList* pMesh, AEGfxTexture* pTexture, float width, float height, float x, float y, float rot);
