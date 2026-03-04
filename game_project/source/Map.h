#pragma once
#include "TMXLoader/TMXLoader.h"
#include "AEEngine.h"
#include "Utils.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

struct RenderNode {
    float y;
    std::function<void()> drawCall;
};

class MapSystem {
public:
    void Init(std::string const& tmxPath, std::string const& tilesetName, std::string const& texturePath);

    // Standard fast draw for flat background layers (like the floor)
    void Draw(std::string const& layerName);

    // NEW: Pushes individual tiles into a global queue for depth sorting
    void QueueLayer(std::string const& layerName, std::vector<RenderNode>& renderQueue);

    void Unload();

    // Builds a binary collision grid from a named tile layer.
    // Call once after Init(). Any tile with a non-zero GID becomes solid (1).
    // Pass the name of your wall/collision layer as it appears in Tiled.
    void BuildCollisionGrid(std::string const& collisionLayerName);

    // Returns true if the screen-space world position (worldX, worldY) falls
    // inside a solid tile, or outside the map boundary.
    // Uses the correct isometric inverse transform accounting for the 180-degree
    // flip and the -10 render offset applied in Draw() / QueueLayer().
    bool isSolid(float worldX, float worldY) const;

    // Returns true if a bounding box centred at (worldX, worldY) with the
    // given screen-space radius overlaps any solid tile.  Used by knockback
    // bounce logic where wall-sliding (ResolveCollision) is not wanted.
    bool IsPositionBlocked(float worldX, float worldY, float radius) const;

    unsigned GetMapWidth() const;
    unsigned GetMapHeight() const;

private:
    TMXLoader m_loader;
    TMXMap* m_currentMap = nullptr;
    AEGfxTexture* m_tilesetTex = nullptr;

    // Maps a Tile GID to a specific mesh with correct UV coordinates
    std::unordered_map<unsigned, AEGfxVertexList*> m_tileMeshes;

    // Binary collision grid: m_collisionGrid[row][col]
    // 0 = walkable, 1 = solid wall. Indexed in TMX (row, col) order.
    std::vector<std::vector<uint8_t>> m_collisionGrid;
    unsigned m_gridWidth  = 0;
    unsigned m_gridHeight = 0;

    // Helper to generate a mesh for a specific sub-region of a texture
    AEGfxVertexList* CreateTileMesh(float u, float v, float u_width, float v_height, u32 color);
};

// ---------------------------------------------------------------------------
// Free utility: resolve an entity move against the isometric collision grid.
//
// Tests the desired displacement (velX, velY) against the grid and modifies
// posX / posY to the largest allowed move using wall-sliding:
//   1. Full move clear  → apply both components.
//   2. Full move blocked → try X-only and Y-only independently.
//      • Screen-space X (left/right) slides along the iso NE-SW wall face.
//      • Screen-space Y (up/down)    slides along the iso NW-SE wall face.
//
// radius – the entity's screen-space collision radius (e.g. m_Size).
// ---------------------------------------------------------------------------
void ResolveCollision(float& posX, float& posY,
                      float velX, float velY,
                      float radius,
                      const MapSystem& map);

// ---------------------------------------------------------------------------
// Returns a random walkable world position that is at least minDist pixels
// away from playerPos.  Retries up to 100 times; falls back to map centre.
// ---------------------------------------------------------------------------
AEVec2 GetRandomSpawnPos(const MapSystem& map, const AEVec2& playerPos,
                         float minDist, float enemyRadius);