/*************************************************************************
@file       Raycast.cpp
@Author     Nigel Lim, nigelkaiyu.lim@digipen.edu
@Co-authors nil
@brief      This file contains raycasting and line-of-sight utilities,
            including grid-based LOS checks and tile-based ray traversal
            for detecting blocked tiles in the map.

Copyright © 2026 DigiPen, All rights reserved.
*************************************************************************/

#include "pch.h"

#include "Raycast.h"
#include "Map.h"
#include "Utils.h"

namespace {
    // small epsilon for float comparisons (avoid precision issues)
    constexpr f32 EPS = 1e-6f;

    // helper absolute value wrapper for readability
    inline f32 AbsF(f32 v) {
        return std::fabs(v);
    }
}

// returns tile width in world space using adjacent tiles
f32 TileSizeWorldX(MapSystem const& map) {
    AEVec2 tileA = map.TMXToWorld(0, 0);
    AEVec2 tileB = map.TMXToWorld(1, 0);
    return AbsF(tileB.x - tileA.x);
}

// returns tile height in world space using adjacent tiles
f32 TileSizeWorldY(MapSystem const& map) {
    AEVec2 tileA = map.TMXToWorld(0, 0);
    AEVec2 tileB = map.TMXToWorld(0, 1);
    return AbsF(tileB.y - tileA.y);
}

// grid-based line of sight check between two world positions
// uses a Bresenham-style stepping across tiles
bool HasLineOfSight_Grid(AEVec2 const& from, AEVec2 const& to, MapSystem const& map) {
    GridPos startTile = map.WorldToTMX(from.x, from.y);
    GridPos endTile = map.WorldToTMX(to.x, to.y);

    int x = startTile.col;
    int y = startTile.row;
    int x1 = endTile.col;
    int y1 = endTile.row;

    // if either tile is not walkable, LOS is blocked
    if (!map.IsWalkable(x, y)) { return false; }
    if (!map.IsWalkable(x1, y1)) { return false; }

    int dx = std::abs(x1 - x);
    int dy = std::abs(y1 - y);

    // determine stepping direction
    int stepX = (x < x1) ? 1 : (x > x1 ? -1 : 0);
    int stepY = (y < y1) ? 1 : (y > y1 ? -1 : 0);

    // same tile -> immediate LOS
    if (dx == 0 && dy == 0) { return true; }

    int error = dx - dy;

    while (!(x == x1 && y == y1)) {
        int oldX = x;
        int oldY = y;

        int error2 = error * 2;

        bool movedX = false;
        bool movedY = false;

        // horizontal step
        if (error2 > -dy) {
            error -= dy;
            x += stepX;
            movedX = (stepX != 0);
        }

        // vertical step
        if (error2 < dx) {
            error += dx;
            y += stepY;
            movedY = (stepY != 0);
        }

        // check current tile
        if (!map.IsWalkable(x, y)) { return false; }

        // handle corner crossing (prevents clipping through corners)
        if (movedX && movedY) {
            if (!map.IsWalkable(oldX, y)) { return false; }
            if (!map.IsWalkable(x, oldY)) { return false; }
        }
    }

    return true;
}

// casts a ray until it hits a blocked tile or reaches max distance
RaycastHit RaycastToBlockedTile(AEVec2 const& from, AEVec2 const& dirNorm,
    f32 maxDist, f32 radius,
    MapSystem const& map) {

    RaycastHit result{};
    result.hit = false;

    // assume no hit initially (full distance)
    result.t = maxDist;
    result.point = { from.x + dirNorm.x * maxDist, from.y + dirNorm.y * maxDist };
    result.tile = map.WorldToTMX(result.point.x, result.point.y);

    // normalize direction if needed
    AEVec2 rayDir = dirNorm;
    f32 dirLength = AEVec2Length(&rayDir);

    if (dirLength <= EPS) {
        return result;
    }

    if (AbsF(dirLength - 1.0f) > 0.01f) {
        AEVec2Normalize(&rayDir, &rayDir);
    }

    // calculate tile size
    const f32 tileWidth = TileSizeWorldX(map);
    const f32 tileHeight = TileSizeWorldY(map);

    if (tileWidth <= EPS || tileHeight <= EPS) {
        return result;
    }

    const f32 halfTileWidth = tileWidth * 0.5f;
    const f32 halfTileHeight = tileHeight * 0.5f;

    GridPos startTile = map.WorldToTMX(from.x, from.y);
    int tileX = startTile.col;
    int tileY = startTile.row;

    // determine stepping direction
    const int stepX = (rayDir.x > 0.0f) ? 1 : (rayDir.x < 0.0f ? -1 : 0);
    const int stepY = (rayDir.y > 0.0f) ? 1 : (rayDir.y < 0.0f ? -1 : 0);

    AEVec2 tileCenter = map.TMXToWorld(tileX, tileY);

    // next tile boundary
    const f32 nextBoundaryX = (stepX > 0) ? (tileCenter.x + halfTileWidth) : (tileCenter.x - halfTileWidth);
    const f32 nextBoundaryY = (stepY > 0) ? (tileCenter.y + halfTileHeight) : (tileCenter.y - halfTileHeight);

    // initial t values for boundary crossing
    f32 tMaxX = (stepX == 0) ? FLT_MAX : ((nextBoundaryX - from.x) / rayDir.x);
    f32 tMaxY = (stepY == 0) ? FLT_MAX : ((nextBoundaryY - from.y) / rayDir.y);

    // delta t for stepping between boundaries
    f32 tDeltaX = (stepX == 0) ? FLT_MAX : (tileWidth / AbsF(rayDir.x));
    f32 tDeltaY = (stepY == 0) ? FLT_MAX : (tileHeight / AbsF(rayDir.y));

    f32 t = 0.0f;

    // if starting inside a wall
    if (map.IsPositionBlocked(from.x, from.y, radius)) {
        result.hit = true;
        result.t = 0.0f;
        result.point = from;
        result.tile = startTile;
        return result;
    }

    // step through tiles using DDA
    while (t <= maxDist) {

        // corner case: step both axes
        if (AbsF(tMaxX - tMaxY) <= EPS) {
            tileX += stepX;
            tileY += stepY;
            t = tMaxX;
            tMaxX += tDeltaX;
            tMaxY += tDeltaY;
        }
        else if (tMaxX < tMaxY) {
            tileX += stepX;
            t = tMaxX;
            tMaxX += tDeltaX;
        }
        else {
            tileY += stepY;
            t = tMaxY;
            tMaxY += tDeltaY;
        }

        AEVec2 samplePoint{ from.x + rayDir.x * t,
                           from.y + rayDir.y * t };

        // collision check against map
        if (map.IsPositionBlocked(samplePoint.x, samplePoint.y, radius)) {
            result.hit = true;
            result.t = t;
            result.point = samplePoint;
            result.tile = { tileX, tileY };
            return result;
        }
    }

    return result;
}