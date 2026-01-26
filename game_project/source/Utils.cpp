#include "Utils.h"

// Tile Basis Vectors
const float I_X = 1.0f;
const float I_Y = 1.0f;
const float J_X = -1.0f;
const float J_Y = 1.0f;

Vec2 GridToScreen(int gridX, int gridY) {
    Vec2 result;
    float halfW = GRID_W * 0.5f;
    float halfH = GRID_H * 0.5f;
    result.x = (gridX * I_X * halfW) + (gridY * J_X * halfW);
    result.y = (gridX * I_Y * halfH) + (gridY * J_Y * halfH);
    return result;
}

bool AreCirclesIntersecting(float c1_x, float c1_y, float r1, float c2_x, float c2_y, float r2) {
    return (sqrt(pow((c1_x - c2_x), 2) + pow((c1_y - c2_y), 2)) < (r1 + r2));
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
    AEGfxTriAdd(0.0f, 0.5f, color, 0.0f, 0.0f, 0.0f, -0.5f, color, 0.0f, 1.0f, 1.0f, -0.5f, color, 1.0f, 1.0f);
    AEGfxTriAdd(0.0f, 0.5f, color, 0.0f, 0.0f, 1.0f, -0.5f, color, 1.0f, 1.0f, 1.0f, 0.5f, color, 1.0f, 0.0f);
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