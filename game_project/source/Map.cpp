#include "pch.h"
#include "Map.h"
#include <iostream>

Map::Map() : m_currentMap(nullptr) {}
Map::~Map() {}

void Map::LoadMap(const std::string& mapName, const std::string& filePath) {
    m_tmxLoader.loadMap(mapName, filePath);
    m_currentMap = m_tmxLoader.getMap(mapName);

    if (m_currentMap != nullptr) {
        std::cout << "Successfully loaded map: " << mapName << std::endl;
    }
    else {
        std::cout << "Failed to load map: " << mapName << std::endl;
    }
}

TMXMap* Map::GetMapData() const {
    return m_currentMap;
}