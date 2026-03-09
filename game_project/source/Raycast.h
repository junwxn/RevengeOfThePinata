#pragma once
#include "Utils.h"
#include "Map.h"

struct RaycastHit {
    bool hit{};
    AEVec2 point{};
    GridPos tile{};
    f32 t{};
};

f32 TileSizeWorldX(MapSystem const& map);
f32 TileSizeWorldY(MapSystem const& map);
bool HasLineOfSight_Grid(AEVec2 const& from, AEVec2 const& to, MapSystem const& map);
RaycastHit RaycastToBlockedTile(AEVec2 const& from, AEVec2 const& dirNorm, f32 maxDist, f32 radius, MapSystem const& map);
