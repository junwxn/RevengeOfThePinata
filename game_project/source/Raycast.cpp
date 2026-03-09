#include "pch.h"

#include "Raycast.h"
#include "Map.h"
#include "Utils.h"

namespace {
    // Small number used when comparing floating point values.
    // Prevents divide-by-zero or precision issues.
    constexpr f32 EPS = 1e-6f;

    // Returns absolute value of a float.
    // Wrapper to make the code shorter.
    inline f32 AbsF(f32 v) {
        return std::fabs(v);
    }
}

// Returns the width of a tile in world space.
// It checks the distance between two neighbouring tiles.
f32 TileSizeWorldX(MapSystem const& map) {
    AEVec2 tileA = map.TMXToWorld(0, 0);
    AEVec2 tileB = map.TMXToWorld(1, 0);
    return AbsF(tileB.x - tileA.x);
}

// Returns the height of a tile in world space.
f32 TileSizeWorldY(MapSystem const& map) {
    AEVec2 tileA = map.TMXToWorld(0, 0);
    AEVec2 tileB = map.TMXToWorld(0, 1);
    return AbsF(tileB.y - tileA.y);
}

// Checks if two positions can see each other without walls blocking the view.
// The algorithm walks tile by tile from the start tile to the end tile.
bool HasLineOfSight_Grid(AEVec2 const& from, AEVec2 const& to, MapSystem const& map) {
    GridPos startTile = map.WorldToTMX(from.x, from.y);
    GridPos endTile = map.WorldToTMX(to.x, to.y);

    int x = startTile.col;
    int y = startTile.row;
    int x1 = endTile.col;
    int y1 = endTile.row;

    // If start or end tile is a wall, there is no line of sight.
    if (!map.IsWalkable(x, y)) { return false; }
    if (!map.IsWalkable(x1, y1)) { return false; }

    int dx = std::abs(x1 - x);
    int dy = std::abs(y1 - y);

    // Direction the line moves in the grid.
    // If tiles are equal, step becomes 0.
    int stepX = (x < x1) ? 1 : (x > x1 ? -1 : 0);
    int stepY = (y < y1) ? 1 : (y > y1 ? -1 : 0);

    // If both points are in the same tile, LOS is clear.
    if (dx == 0 && dy == 0) { return true; }

    int error = dx - dy;

    while (!(x == x1 && y == y1)) {
        int oldX = x;
        int oldY = y;

        int error2 = error * 2;

        bool movedX = false;
        bool movedY = false;

        // Move horizontally if the line crosses a vertical boundary.
        if (error2 > -dy) {
            error -= dy;
            x += stepX;
            movedX = (stepX != 0);
        }

        // Move vertically if the line crosses a horizontal boundary.
        if (error2 < dx) {
            error += dx;
            y += stepY;
            movedY = (stepY != 0);
        }

        // Check the tile we moved into.
        if (!map.IsWalkable(x, y)) { return false; }

        // If the line crossed a corner, also check the two side tiles.
        // This prevents walls from being skipped at corners.
        if (movedX && movedY) {
            if (!map.IsWalkable(oldX, y)) { return false; }
            if (!map.IsWalkable(x, oldY)) { return false; }
        }
    }

    return true;
}

// Casts a ray until it hits a wall tile or reaches max distance.
// Returns where the ray stopped.
RaycastHit RaycastToBlockedTile(AEVec2 const& from, AEVec2 const& dirNorm,
    f32 maxDist, f32 radius,
    MapSystem const& map) {

    RaycastHit result{};
    result.hit = false;

    // Assume the ray travels full distance unless a wall is found.
    result.t = maxDist;
    result.point = { from.x + dirNorm.x * maxDist, from.y + dirNorm.y * maxDist };
    result.tile = map.WorldToTMX(result.point.x, result.point.y);

    // Ensure the direction vector is normalized.
    AEVec2 rayDir = dirNorm;
    f32 dirLength = AEVec2Length(&rayDir);

    if (dirLength <= EPS) {
        return result;
    }

    if (AbsF(dirLength - 1.0f) > 0.01f) {
        AEVec2Normalize(&rayDir, &rayDir);
    }

    // Estimate tile size in world space.
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

    // Determine which direction the ray moves in the grid.
    const int stepX = (rayDir.x > 0.0f) ? 1 : (rayDir.x < 0.0f ? -1 : 0);
    const int stepY = (rayDir.y > 0.0f) ? 1 : (rayDir.y < 0.0f ? -1 : 0);

    AEVec2 tileCenter = map.TMXToWorld(tileX, tileY);

    // Position of the next grid boundary in each direction.
    const f32 nextBoundaryX = (stepX > 0) ? (tileCenter.x + halfTileWidth) : (tileCenter.x - halfTileWidth);
    const f32 nextBoundaryY = (stepY > 0) ? (tileCenter.y + halfTileHeight) : (tileCenter.y - halfTileHeight);

    // Distance along the ray until the next boundary is reached.
    f32 tMaxX = (stepX == 0) ? FLT_MAX : ((nextBoundaryX - from.x) / rayDir.x);
    f32 tMaxY = (stepY == 0) ? FLT_MAX : ((nextBoundaryY - from.y) / rayDir.y);

    // Distance between each boundary crossing.
    f32 tDeltaX = (stepX == 0) ? FLT_MAX : (tileWidth / AbsF(rayDir.x));
    f32 tDeltaY = (stepY == 0) ? FLT_MAX : (tileHeight / AbsF(rayDir.y));

    f32 t = 0.0f;

    // If the starting position is already inside a wall.
    if (map.IsPositionBlocked(from.x, from.y, radius)) {
        result.hit = true;
        result.t = 0.0f;
        result.point = from;
        result.tile = startTile;
        return result;
    }

    // Step through tiles until a wall is found.
    while (t <= maxDist) {

        // When the ray hits a corner, step both directions.
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

        // Check if the ray collided with a blocked tile.
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