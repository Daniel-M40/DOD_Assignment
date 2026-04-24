#include "spatial_hash.h"
#include <algorithm>

SpatialHash::SpatialHash(float cellSize, float worldSizeX, float worldSizeY)
    : mCellSize(cellSize), mWorldSizeX(worldSizeX), mWorldSizeY(worldSizeY)
{
    mGridSizeX = static_cast<int>((worldSizeX * 2.0f) / cellSize) + 1;
    mGridSizeY = static_cast<int>((worldSizeY * 2.0f) / cellSize) + 1;
    mCells.resize(mGridSizeX * mGridSizeY);
}

void SpatialHash::Insert(float x, float y, uint32_t circleIndex)
{
    uint32_t cellIndex = PositionToCellIndex(x, y);
    mCells[cellIndex].push_back(circleIndex);
}

void SpatialHash::Query(float x, float y, float radius, std::function<void(uint32_t)> callback)
{
    int minCellX = static_cast<int>((x - radius + mWorldSizeX) / mCellSize);
    int maxCellX = static_cast<int>((x + radius + mWorldSizeX) / mCellSize);
    int minCellY = static_cast<int>((y - radius + mWorldSizeY) / mCellSize);
    int maxCellY = static_cast<int>((y + radius + mWorldSizeY) / mCellSize);

    minCellX = std::max(minCellX, 0);
    maxCellX = std::min(maxCellX, mGridSizeX - 1);
    minCellY = std::max(minCellY, 0);
    maxCellY = std::min(maxCellY, mGridSizeY - 1);

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
}

void SpatialHash::Clear()
{
    for (auto& cell : mCells)
    {
        cell.clear();
    }
}

uint32_t SpatialHash::PositionToCellIndex(float x, float y) const
{
    int cellX = static_cast<int>((x + mWorldSizeX) / mCellSize);
    int cellY = static_cast<int>((y + mWorldSizeY) / mCellSize);

    cellX = std::max(0, std::min(cellX, mGridSizeX - 1));
    cellY = std::max(0, std::min(cellY, mGridSizeY - 1));

    return cellX + cellY * mGridSizeX;
}