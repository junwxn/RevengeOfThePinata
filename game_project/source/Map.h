#pragma once
#include "TMXLoader/TMXLoader.h"
#include "AEEngine.h"
#include "Utils.h"
#include <string>
#include <unordered_map>

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

    unsigned GetMapWidth() const;
    unsigned GetMapHeight() const;

private:
    TMXLoader m_loader;
    TMXMap* m_currentMap = nullptr;
    AEGfxTexture* m_tilesetTex = nullptr;

    // Maps a Tile GID to a specific mesh with correct UV coordinates
    std::unordered_map<unsigned, AEGfxVertexList*> m_tileMeshes;

    // Helper to generate a mesh for a specific sub-region of a texture
    AEGfxVertexList* CreateTileMesh(float u, float v, float u_width, float v_height, u32 color);
};