#include "spatial_hash.h"
#include <algorithm>

#ifdef ENABLE_3D
SpatialHash::SpatialHash(float cellSize, float worldSizeX, float worldSizeY, float worldSizeZ)
    : mCellSize(cellSize), mWorldSizeX(worldSizeX), mWorldSizeY(worldSizeY), mWorldSizeZ(worldSizeZ)
{
    mGridSizeX = static_cast<int>((worldSizeX * 2.0f) / cellSize) + 1;
    mGridSizeY = static_cast<int>((worldSizeY * 2.0f) / cellSize) + 1;
    mGridSizeZ = static_cast<int>((worldSizeZ * 2.0f) / cellSize) + 1;
    mCells.resize(mGridSizeX * mGridSizeY * mGridSizeZ);
}
#else
SpatialHash::SpatialHash(float cellSize, float worldSizeX, float worldSizeY)
    : mCellSize(cellSize), mWorldSizeX(worldSizeX), mWorldSizeY(worldSizeY)
{
    mGridSizeX = static_cast<int>((worldSizeX * 2.0f) / cellSize) + 1;
    mGridSizeY = static_cast<int>((worldSizeY * 2.0f) / cellSize) + 1;
    mCells.resize(mGridSizeX * mGridSizeY);
}
#endif

void SpatialHash::Insert(float x, float y,
#ifdef ENABLE_3D
    float z,
#endif
    uint32_t circleIndex)
{
    uint32_t cellIndex = PositionToCellIndex(x, y
#ifdef ENABLE_3D
        , z
#endif
    );
    mCells[cellIndex].push_back(circleIndex);
}

void SpatialHash::Query(float x, float y,
#ifdef ENABLE_3D
    float z,
#endif
    float radius, std::function<void(uint32_t)> callback)
{
    int minCellX = static_cast<int>((x - radius + mWorldSizeX) / mCellSize);
    int maxCellX = static_cast<int>((x + radius + mWorldSizeX) / mCellSize);
    int minCellY = static_cast<int>((y - radius + mWorldSizeY) / mCellSize);
    int maxCellY = static_cast<int>((y + radius + mWorldSizeY) / mCellSize);

    minCellX = std::max(minCellX, 0);
    maxCellX = std::min(maxCellX, mGridSizeX - 1);
    minCellY = std::max(minCellY, 0);
    maxCellY = std::min(maxCellY, mGridSizeY - 1);

#ifdef ENABLE_3D
    int minCellZ = static_cast<int>((z - radius + mWorldSizeZ) / mCellSize);
    int maxCellZ = static_cast<int>((z + radius + mWorldSizeZ) / mCellSize);
    minCellZ = std::max(minCellZ, 0);
    maxCellZ = std::min(maxCellZ, mGridSizeZ - 1);

    for (int cz = minCellZ; cz <= maxCellZ; ++cz)
    {
        for (int cy = minCellY; cy <= maxCellY; ++cy)
        {
            for (int cx = minCellX; cx <= maxCellX; ++cx)
            {
                const auto& cell = mCells[cx + cy * mGridSizeX + cz * mGridSizeX * mGridSizeY];
                for (uint32_t index : cell)
                {
                    callback(index);
                }
            }
        }
    }
#else
    for (int cy = minCellY; cy <= maxCellY; ++cy)
    {
        for (int cx = minCellX; cx <= maxCellX; ++cx)
        {
            const auto& cell = mCells[cx + cy * mGridSizeX];
            for (uint32_t index : cell)
            {
                callback(index);
            }
        }
    }
#endif
}

void SpatialHash::Clear()
{
    for (auto& cell : mCells)
    {
        cell.clear();
    }
}

uint32_t SpatialHash::PositionToCellIndex(float x, float y
#ifdef ENABLE_3D
    , float z
#endif
) const
{
    int cellX = static_cast<int>((x + mWorldSizeX) / mCellSize);
    int cellY = static_cast<int>((y + mWorldSizeY) / mCellSize);

    cellX = std::max(0, std::min(cellX, mGridSizeX - 1));
    cellY = std::max(0, std::min(cellY, mGridSizeY - 1));

#ifdef ENABLE_3D
    int cellZ = static_cast<int>((z + mWorldSizeZ) / mCellSize);
    cellZ = std::max(0, std::min(cellZ, mGridSizeZ - 1));
    return cellX + cellY * mGridSizeX + cellZ * mGridSizeX * mGridSizeY;
#else
    return cellX + cellY * mGridSizeX;
#endif
}