/*************************************************************************
@file       Raycast.h
@Author     Nigel Lim, nigelkaiyu.lim@digipen.edu
@Co-authors nil
@brief      This file declares raycasting and line-of-sight utilities
            used for detecting visibility and collisions with blocked
            tiles in the map.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#pragma once
#include "Utils.h"
#include "Map.h"

// stores result of a raycast
struct RaycastHit {
    bool hit{};        // whether the ray hit a blocked tile
    AEVec2 point{};    // world position where the ray stopped
    GridPos tile{};    // grid tile that was hit
    f32 t{};           // distance travelled along the ray
};

f32 TileSizeWorldX(MapSystem const& map);
f32 TileSizeWorldY(MapSystem const& map);
bool HasLineOfSight_Grid(AEVec2 const& from, AEVec2 const& to, MapSystem const& map);
RaycastHit RaycastToBlockedTile(AEVec2 const& from, AEVec2 const& dirNorm, f32 maxDist, f32 radius, MapSystem const& map);