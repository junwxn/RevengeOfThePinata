#pragma once
#include "TMXLoader/TMXLoader.h"
#include "AEEngine.h"
#include "Utils.h"
#include <string>
#include <unordered_map>

class MapSystem {
public:
    // Initialize the map, load the texture, and generate UV meshes
    void Init(std::string const& tmxPath, std::string const& tilesetName, std::string const& texturePath);

    // Draw a specific layer
    void Draw(std::string const& layerName);

    // Unload textures and free all generated tile meshes
    void Unload();

private:
    TMXLoader m_loader;
    TMXMap* m_currentMap = nullptr;
    AEGfxTexture* m_tilesetTex = nullptr;

    // Maps a Tile GID to a specific mesh with correct UV coordinates
    std::unordered_map<unsigned, AEGfxVertexList*> m_tileMeshes;

    // Helper to generate a mesh for a specific sub-region of a texture
    AEGfxVertexList* CreateTileMesh(float u, float v, float u_width, float v_height, u32 color);
};