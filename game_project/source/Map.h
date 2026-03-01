#pragma once
#include <string>
#include "TMXLoader/TMXLoader.h"

class Map {
public:
    Map();
    ~Map();

    void LoadMap(const std::string& mapName, const std::string& filePath);
    TMXMap* GetMapData() const;

private:
    TMXLoader m_tmxLoader;
    TMXMap* m_currentMap;
};