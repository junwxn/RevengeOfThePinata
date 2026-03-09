#include "pch.h"

#include "Utils.h"

// Tile Basis Vectors
const float I_X = 1.0f;
const float I_Y = 1.0f;
const float J_X = -1.0f;
const float J_Y = 1.0f;

Vec2 GridToScreen(int gridX, int gridY) {
    Vec2 result{};
    float halfW = GRID_W * 0.5f;
    float halfH = GRID_H * 0.5f;
    result.x = (gridX * I_X * halfW) + (gridY * J_X * halfW);
    result.y = (gridX * I_Y * halfH) + (gridY * J_Y * halfH);
    return result;
}

bool AreCirclesIntersecting(float c1_x, float c1_y, float r1, float c2_x, float c2_y, float r2) {
    return (sqrt(pow((c1_x - c2_x), 2) + pow((c1_y - c2_y), 2)) < (r1 + r2));
}

bool IsMouseInside(float mousepos_x, float mousepos_y, float center_x, float center_y, float width, float height)
{ // same as /2, but *0.5 is faster
    return (mousepos_x > center_x - (width * 0.5f) && // left
        mousepos_x < center_x + (width * 0.5f) && // right
        mousepos_y > center_y - (height * 0.5f) && // top
        mousepos_y < center_y + (height * 0.5f)); // bottom
}

AEGfxVertexList* CreateCircleMesh(f32 radius, u8 parts, u32 color) {
    AEGfxMeshStart();
    f32 angleStep = (2.0f * PI) / parts;
    f32 currentAngle = 0.0f;

    for (int i = 0; i < parts; ++i) {
        f32 x1 = radius * cosf(currentAngle);
        f32 y1 = radius * sinf(currentAngle);
        f32 x2 = radius * cosf(currentAngle + angleStep);
        f32 y2 = radius * sinf(currentAngle + angleStep);

        AEGfxTriAdd(
            0.0f, 0.0f, color, 0.0f, 0.0f,
            x1, y1, color, 0.0f, 0.0f,
            x2, y2, color, 0.0f, 0.0f
        );
        currentAngle += angleStep;
    }
    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateRectMesh(u32 color) {
    AEGfxMeshStart();
    // Isometric Left-Anchored Rectangle
    AEGfxTriAdd(0.0f, 0.5f, color, 0.0f, 0.0f, 
                0.0f, -0.5f, color, 0.0f, 1.0f, 
                1.0f, -0.5f, color, 1.0f, 1.0f);
    AEGfxTriAdd(0.0f, 0.5f, color, 0.0f, 0.0f, 
                1.0f, -0.5f, color, 1.0f, 1.0f, 
                1.0f, 0.5f, color, 1.0f, 0.0f);
    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateSpriteRectMesh(u32 color, float newU, float newV) {
    const float u = 1.0f / newU;
    const float v = 1.0f / newV;

    AEGfxMeshStart();

    AEGfxTriAdd(
        -0.5f, 0.5f, color, 0.0f, 0.0f,
        -0.5f, -0.5f, color, 0.0f, v,
        0.5f, -0.5f, color, u, v);

    AEGfxTriAdd(
        -0.5f, 0.5f, color, 0.0f, 0.0f,
        0.5f, -0.5f, color, u, v,
        0.5f, 0.5f, color, u, 0.0f);

    return AEGfxMeshEnd();
}


AEGfxVertexList* CreateTriangleMesh(u32 color) {
    AEGfxMeshStart();
    AEGfxTriAdd( // Triangle 1
        0.0f, 0.5f, color, 0.0f, 0.0f,  // left-top
        0.0f, -0.5f, color, 0.0f, 0.0f,   // left-bottom
        -0.5f, 0.5f, color, 0.0f, 0.0f);  // right-top
    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateLineMesh(float range, u32 color) {
    AEGfxMeshStart();
    // Isometric Left-Anchored Rectangle
    AEGfxTriAdd(0.0f, 0.5f, color, 0.0f, 0.0f, 0.0f, -0.5f, color, 0.0f, 1.0f, 1.0f, -0.5f, color, 1.0f, 1.0f);
    AEGfxTriAdd(0.0f, 0.5f, color, 0.0f, 0.0f, 1.0f, -0.5f, color, 1.0f, 1.0f, 1.0f, 0.5f, color, 1.0f, 0.0f);
    AEGfxTriAdd(0.0f, 0.5f, color, 0, 0, 0.0f, -0.5f, color, 0, 0, range, 0.5f, color, 0, 0);
    AEGfxTriAdd(range, 0.5f, color, 0, 0,range, -0.5f, color, 0, 0,0.0f, -0.5f, color, 0, 0);
    return AEGfxMeshEnd();
}


AEGfxVertexList* CreateAttackRangeMesh(f32 attackRange, u32 color) {
    AEGfxMeshStart();
    AEGfxTriAdd(
        0.0f, 0.5f, color, 0, 0,
        0.0f, -0.5f, color, 0, 0,
        -attackRange, 0.5f, color, 0, 0);

    AEGfxTriAdd(
        -attackRange, 0.5f, color, 0, 0,
        -attackRange, -0.5f, color, 0, 0,
        0.0f, -0.5f, color, 0, 0);
    return AEGfxMeshEnd();
}

AEGfxVertexList* CreateRingMesh(int segments, f32 thickness) {
    const f32 outer = 1.0f;
    const f32 inner = outer - thickness;

    AEGfxMeshStart();

    f32 step = (PI * 2.0f) / segments;

    for (int i = 0; i < segments; ++i) {
        f32 a0 = i * step;
        f32 a1 = (i + 1) * step;

        f32 x0 = cosf(a0);
        f32 y0 = sinf(a0);
        f32 x1 = cosf(a1);
        f32 y1 = sinf(a1);

        f32 ox0 = x0 * outer;
        f32 oy0 = y0 * outer;
        f32 ox1 = x1 * outer;
        f32 oy1 = y1 * outer;

        f32 ix0 = x0 * inner;
        f32 iy0 = y0 * inner;
        f32 ix1 = x1 * inner;
        f32 iy1 = y1 * inner;

        AEGfxTriAdd(
            ox0, oy0, 0xFFFFFFFF, 0, 0,
            ox1, oy1, 0xFFFFFFFF, 0, 0,
            ix1, iy1, 0xFFFFFFFF, 0, 0
        );

        AEGfxTriAdd(
            ox0, oy0, 0xFFFFFFFF, 0, 0,
            ix1, iy1, 0xFFFFFFFF, 0, 0,
            ix0, iy0, 0xFFFFFFFF, 0, 0
        );
    }

    return AEGfxMeshEnd();
}

void DrawMesh(AEGfxVertexList* pMesh, float width, float height, float x, float y, float rot, float r, float g, float b, float a) {
    AEGfxSetColorToMultiply(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetColorToAdd(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Rot(&rotate, rot);
    AEMtx33Trans(&translate, x, y);

    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}

void DrawTexture(Sprite& spriteObj, int currentDirection, AEGfxVertexList* pMesh, AEGfxTexture* pTexture, float width, float height, float x, float y, float rot) {
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    spriteObj.SetTextureU();
    spriteObj.SetTextureV(currentDirection);

    AEGfxTextureSet(pTexture, spriteObj.GetU(), spriteObj.GetV());

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Rot(&rotate, rot);
    AEMtx33Trans(&translate, x, y);

    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}

void DrawTexturePlayer(Sprite& spriteObj, int currentDirection,
    AEGfxVertexList* pMesh, AEGfxTexture* pTexture,
    float width, float height, float x, float y, float rot)
{
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1, 1, 1, 1);
    AEGfxSetColorToAdd(0, 0, 0, 0);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);

    spriteObj.SetTexturePlayerU();
    spriteObj.SetTexturePlayerV(currentDirection);

    AEGfxTextureSet(pTexture, spriteObj.GetPlayerU(), spriteObj.GetPlayerV());

    AEMtx33 scale, rotate, translate, transform;
    AEMtx33Scale(&scale, width, height);
    AEMtx33Rot(&rotate, rot);
    AEMtx33Trans(&translate, x, y);

    AEMtx33Concat(&transform, &rotate, &scale);
    AEMtx33Concat(&transform, &translate, &transform);

    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}