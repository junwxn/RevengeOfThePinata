#include "pch.h"
#include "Map.h"
#include <cmath>

void MapSystem::QueueLayer(std::string const& layerName, std::vector<RenderNode>& renderQueue) {
    if (!m_currentMap || !m_tilesetTex) return;

    TMXTileLayer* layer = m_currentMap->getLayer(layerName);
    if (!layer) return;

    auto tiles = layer->getTiles();
    unsigned mapW = layer->getWidth();
    unsigned mapH = layer->getHeight();

    for (int y = 0; y < (int)mapH; ++y) {
        for (int x = 0; x < (int)mapW; ++x) {
            unsigned gid = tiles[y][x];
            if (gid == 0) continue;

            int renderX = (mapW - 1) - x;
            int renderY = (mapH - 1) - y;
            Vec2 pos = GridToScreen(renderX - 10, renderY - 10);

            if (m_tileMeshes.find(gid) != m_tileMeshes.end()) {

                // Extract variables to safely pass into the lambda
                AEGfxVertexList* mesh = m_tileMeshes[gid];
                AEGfxTexture* tex = m_tilesetTex;
                float posX = pos.x;
                float posY = pos.y;

                // Push a custom draw command into the global list
                renderQueue.push_back({
                    posY, // Sorting variable
                    [mesh, tex, posX, posY]() { // The actual draw logic
                        // Re-apply texture states in case a player/enemy changed them
                        AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
                        AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
                        AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
                        AEGfxSetBlendMode(AE_GFX_BM_BLEND);
                        AEGfxSetTransparency(1.0f);
                        AEGfxTextureSet(tex, 0.0f, 0.0f);

                        AEMtx33 scale, trans, transform;
                        AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);
                        AEMtx33Trans(&trans, posX, posY);
                        AEMtx33Concat(&transform, &trans, &scale);

                        AEGfxSetTransform(transform.m);
                        AEGfxMeshDraw(mesh, AE_GFX_MDM_TRIANGLES);
                    }
                    });
            }
        }
    }
}

void MapSystem::Init(std::string const& tmxPath, std::string const& tilesetName, std::string const& texturePath) {
    // 1. Load the map and texture
    m_loader.loadMap("Level1Map", tmxPath);
    m_currentMap = m_loader.getMap("Level1Map");
    m_tilesetTex = AEGfxTextureLoad(texturePath.c_str());

    if (!m_currentMap) return;

    // 2. Extract tileset data to calculate UVs
    TMXTileSet* ts = m_currentMap->getTileset(tilesetName);
    if (!ts) return; // Note: Ensure 'tilesetName' exactly matches the name in Tiled

    float imgW = (float)ts->getImageWidth();
    float imgH = (float)ts->getImageHeight();
    float tileW = (float)ts->getTileWidth();
    float tileH = (float)ts->getTileHeight();
    float spacing = (float)ts->getSpacing();
    float margin = (float)ts->getMargin();
    unsigned firstGid = ts->getFirstGID();
    unsigned tileCount = ts->getTileCount();

    int columns = (int)((imgW - 2 * margin + spacing) / (tileW + spacing));
    if (columns == 0) columns = 1;

    // 3. Pre-generate a mesh for every tile in the tilesheet
    for (unsigned i = 0; i < tileCount; ++i) {
        int col = i % columns;
        int row = i / columns;

        // Calculate normalized UV coordinates (0.0f to 1.0f)
        float u = (margin + col * (tileW + spacing)) / imgW;
        float v = (margin + row * (tileH + spacing)) / imgH;
        float u_w = tileW / imgW;
        float v_h = tileH / imgH;

        // Create the mesh and store it against its unique GID
        m_tileMeshes[firstGid + i] = CreateTileMesh(u, v, u_w, v_h, 0xFFFFFFFF);
    }
}

void MapSystem::Draw(std::string const& layerName) {
    if (!m_currentMap || !m_tilesetTex) return;

    TMXTileLayer* layer = m_currentMap->getLayer(layerName);
    if (!layer) return;

    auto tiles = layer->getTiles();
    unsigned mapW = layer->getWidth();
    unsigned mapH = layer->getHeight();

    // Setup Engine for Texture Rendering
    AEGfxSetRenderMode(AE_GFX_RM_TEXTURE);
    AEGfxSetColorToMultiply(1.0f, 1.0f, 1.0f, 1.0f);
    AEGfxSetColorToAdd(0.0f, 0.0f, 0.0f, 0.0f);
    AEGfxSetBlendMode(AE_GFX_BM_BLEND);
    AEGfxSetTransparency(1.0f);
    AEGfxTextureSet(m_tilesetTex, 0.0f, 0.0f);

    for (int y = 0; y < (int)mapH; ++y) {
        for (int x = 0; x < (int)mapW; ++x) {
            unsigned gid = tiles[y][x];
            if (gid == 0) continue; // 0 represents an empty tile

            // --- The CORRECT 180-Degree Coordinate Flip ---
            int renderX = (mapW - 1) - x;
            int renderY = (mapH - 1) - y;
            Vec2 pos = GridToScreen(renderX - 10, renderY - 10);

            if (m_tileMeshes.find(gid) != m_tileMeshes.end()) {
                AEMtx33 scale, trans, transform;
                AEMtx33Scale(&scale, SPRITE_W, SPRITE_H);
                AEMtx33Trans(&trans, pos.x, pos.y);
                AEMtx33Concat(&transform, &trans, &scale);

                AEGfxSetTransform(transform.m);
                AEGfxMeshDraw(m_tileMeshes[gid], AE_GFX_MDM_TRIANGLES);
            }
        }
    }
}

void MapSystem::Unload() {
    if (m_tilesetTex) {
        AEGfxTextureUnload(m_tilesetTex);
        m_tilesetTex = nullptr;
    }
    // Clean up all generated meshes
    for (auto& pair : m_tileMeshes) {
        AEGfxMeshFree(pair.second);
    }
    m_tileMeshes.clear();
}

// Custom mesh builder that accepts UV parameters
AEGfxVertexList* MapSystem::CreateTileMesh(float u, float v, float u_width, float v_height, u32 color) {
    AEGfxMeshStart();
    // Replicating the Left-Anchored Rectangle from your Utils.cpp
    // Top-Left: (0.0f, 0.5f) -> UV: (u, v)
    // Bottom-Left: (0.0f, -0.5f) -> UV: (u, v + v_height)
    // Bottom-Right: (1.0f, -0.5f) -> UV: (u + u_width, v + v_height)
    // Top-Right: (1.0f, 0.5f) -> UV: (u + u_width, v)

    AEGfxTriAdd(
        0.0f, 0.5f, color, u, v,
        0.0f, -0.5f, color, u, v + v_height,
        1.0f, -0.5f, color, u + u_width, v + v_height
    );
    AEGfxTriAdd(
        0.0f, 0.5f, color, u, v,
        1.0f, -0.5f, color, u + u_width, v + v_height,
        1.0f, 0.5f, color, u + u_width, v
    );
    return AEGfxMeshEnd();
}

unsigned MapSystem::GetMapWidth() const {
    if (m_currentMap) {
        return m_currentMap->getWidth();
    }
    return 0;
}

unsigned MapSystem::GetMapHeight() const {
    if (m_currentMap) {
        return m_currentMap->getHeight();
    }
    return 0;
}

void MapSystem::BuildCollisionGrid(std::string const& collisionLayerName) {
    if (!m_currentMap) return;

    TMXTileLayer* layer = m_currentMap->getLayer(collisionLayerName);
    if (!layer) return;

    m_gridWidth  = layer->getWidth();
    m_gridHeight = layer->getHeight();

    auto tiles = layer->getTiles(); // tiles[row][col], row 0 = top of map

    m_collisionGrid.assign(m_gridHeight, std::vector<uint8_t>(m_gridWidth, 0));

    for (unsigned row = 0; row < m_gridHeight; ++row)
        for (unsigned col = 0; col < m_gridWidth; ++col)
            m_collisionGrid[row][col] = (tiles[row][col] != 0) ? 1u : 0u;
}

bool MapSystem::isSolid(float worldX, float worldY) const {
    if (m_collisionGrid.empty()) return false;

    // --- Isometric inverse transform ---
    //
    // Rendering pipeline (Map.cpp Draw / QueueLayer):
    //   For TMX cell (col, row):
    //     renderX = (mapW - 1) - col
    //     renderY = (mapH - 1) - row
    //   Screen pos = GridToScreen(renderX - RENDER_OFFSET, renderY - RENDER_OFFSET)
    //   GridToScreen(gx, gy):
    //     sx = halfW * (gx - gy)
    //     sy = halfH * (gx + gy)
    //
    // Inverse (sx, sy → gx, gy):
    //   gx = (sx/halfW + sy/halfH) / 2
    //   gy = (-sx/halfW + sy/halfH) / 2
    //
    // Left-anchor correction:
    //   The tile mesh origin is the diamond's LEFT vertex, not its centre.
    //   The visual centre of tile (col,row) maps to iso (n+0.5, m-0.5).
    //   Therefore floor(gy) + 1 gives the correct TMX row after inversion.
    //
    // RENDER_OFFSET (10) must match the constant in Draw() and QueueLayer().

    constexpr int   RENDER_OFFSET = 10;
    const float halfW = GRID_W * 0.5f; // 55.5f
    const float halfH = GRID_H * 0.5f; // 32.0f

    const float isoX = (worldX / halfW + worldY / halfH) * 0.5f;
    const float isoY = (-worldX / halfW + worldY / halfH) * 0.5f;

    // floor + left-anchor +1 correction on the Y axis
    const int tileX = static_cast<int>(std::floorf(isoX));
    const int tileY = static_cast<int>(std::floorf(isoY)) + 1;

    // Undo the render offset and the 180-degree flip to get TMX indices
    const int renderX = tileX + RENDER_OFFSET;
    const int renderY = tileY + RENDER_OFFSET;

    const int tmxCol = static_cast<int>(m_gridWidth)  - 1 - renderX;
    const int tmxRow = static_cast<int>(m_gridHeight) - 1 - renderY;

    // Out-of-bounds treated as solid (entity cannot leave the map)
    if (tmxCol < 0 || tmxRow < 0 ||
        tmxCol >= static_cast<int>(m_gridWidth) ||
        tmxRow >= static_cast<int>(m_gridHeight))
        return true;

    return m_collisionGrid[tmxRow][tmxCol] != 0;
}

// ---------------------------------------------------------------------------
// ResolveCollision implementation
// ---------------------------------------------------------------------------

// Tests whether an axis-aligned bounding box centred at (cx, cy) with
// half-extents (probeX, probeY) overlaps any solid tile.
// Checks 9 sample points: centre, 4 cardinal edges, and 4 diagonal corners.
// The cardinal probes are critical in isometric space because the diamond-shaped
// tiles can slip between the four diagonal corners when approached head-on.
static bool IsBoxSolid(float cx, float cy, float probeX, float probeY,
                        const MapSystem& map)
{
    // Centre
    if (map.isSolid(cx, cy)) return true;
    // 4 cardinal edge midpoints
    if (map.isSolid(cx + probeX, cy)) return true;
    if (map.isSolid(cx - probeX, cy)) return true;
    if (map.isSolid(cx, cy + probeY)) return true;
    if (map.isSolid(cx, cy - probeY)) return true;
    // 4 diagonal corners
    if (map.isSolid(cx + probeX, cy + probeY)) return true;
    if (map.isSolid(cx - probeX, cy + probeY)) return true;
    if (map.isSolid(cx + probeX, cy - probeY)) return true;
    if (map.isSolid(cx - probeX, cy - probeY)) return true;
    return false;
}

void ResolveCollision(float& posX, float& posY,
                      float velX, float velY,
                      float radius,
                      const MapSystem& map)
{
    // Probe half-extents sized to match the isometric entity ellipse.
    // X: full radius covers the wide horizontal extent of the iso sprite.
    // Y: squashed by the iso perspective ratio (GRID_H / GRID_W ≈ 0.58).
    const float probeX = radius * 0.9f;
    const float probeY = radius * (GRID_H / GRID_W) * 0.9f;

    const float newX = posX + velX;
    const float newY = posY + velY;

    // Fast-path: full move is clear — apply it and return.
    if (!IsBoxSolid(newX, newY, probeX, probeY, map)) {
        posX = newX;
        posY = newY;
        return;
    }

    // Full move is blocked.  Test each screen axis independently against the
    // ORIGINAL position so both checks are order-independent.
    //   Screen-X slide → entity glides along the iso NE/SW wall face.
    //   Screen-Y slide → entity glides along the iso NW/SE wall face.
    const bool xClear = !IsBoxSolid(newX, posY, probeX, probeY, map);
    const bool yClear = !IsBoxSolid(posX, newY, probeX, probeY, map);

    if (xClear || yClear) {
        if (xClear) posX = newX;
        if (yClear) posY = newY;
        return;
    }

    // Both screen-axis slides blocked.  Decompose velocity along the two
    // isometric grid axes (NE and NW in screen space) and slide along
    // whichever axis is clear.  This lets the player glide along iso walls
    // even when moving in a pure cardinal screen direction.
    const float halfW = GRID_W * 0.5f;
    const float halfH = GRID_H * 0.5f;
    const float isoLen = sqrtf(halfW * halfW + halfH * halfH);
    const float neX = halfW / isoLen, neY = halfH / isoLen;   // NE axis unit
    const float nwX = -halfW / isoLen, nwY = halfH / isoLen;  // NW axis unit

    // Project velocity onto each iso axis
    const float dotNE = velX * neX + velY * neY;
    const float dotNW = velX * nwX + velY * nwY;

    const float slideNE_posX = posX + dotNE * neX;
    const float slideNE_posY = posY + dotNE * neY;
    const float slideNW_posX = posX + dotNW * nwX;
    const float slideNW_posY = posY + dotNW * nwY;

    const bool neClear = !IsBoxSolid(slideNE_posX, slideNE_posY, probeX, probeY, map);
    const bool nwClear = !IsBoxSolid(slideNW_posX, slideNW_posY, probeX, probeY, map);

    if (neClear) { posX = slideNE_posX; posY = slideNE_posY; }
    else if (nwClear) { posX = slideNW_posX; posY = slideNW_posY; }
    // else: fully stuck against a corner, don't move
}