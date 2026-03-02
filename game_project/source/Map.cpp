#include "pch.h"
#include "Map.h"

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