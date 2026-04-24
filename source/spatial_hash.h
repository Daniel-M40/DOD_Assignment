#pragma once
#include <functional>
#include <vector>
#include <algorithm>
#include <stdint.h>

template<int Dim>
class SpatialHash
{
public:
    SpatialHash(float cellSize, float worldSizeX, float worldSizeY, float worldSizeZ = 0.0f)
        : mCellSize(cellSize), mWorldSizeX(worldSizeX), mWorldSizeY(worldSizeY), mWorldSizeZ(worldSizeZ)
    {
        mGridSizeX = static_cast<int>((worldSizeX * 2.0f) / cellSize) + 1;
        mGridSizeY = static_cast<int>((worldSizeY * 2.0f) / cellSize) + 1;
        mGridSizeZ = 1;

        if (Dim == 3)
        {
            mGridSizeZ = static_cast<int>((worldSizeZ * 2.0f) / cellSize) + 1;
        }

        mCells.resize(mGridSizeX * mGridSizeY * mGridSizeZ);
    }

    void Insert(float x, float y, float z, uint32_t index)
    {
        uint32_t cellIndex = PositionToCellIndex(x, y, z);
        mCells[cellIndex].push_back(index);
    }

    void Query(float x, float y, float z, float radius, std::function<void(uint32_t)> callback)
    {
        int minCellX = static_cast<int>((x - radius + mWorldSizeX) / mCellSize);
        int maxCellX = static_cast<int>((x + radius + mWorldSizeX) / mCellSize);
        int minCellY = static_cast<int>((y - radius + mWorldSizeY) / mCellSize);
        int maxCellY = static_cast<int>((y + radius + mWorldSizeY) / mCellSize);

        minCellX = (std::max)(minCellX, 0);
        maxCellX = (std::min)(maxCellX, mGridSizeX - 1);
        minCellY = (std::max)(minCellY, 0);
        maxCellY = (std::min)(maxCellY, mGridSizeY - 1);

        int minCellZ = 0;
        int maxCellZ = 0;

        if (Dim == 3)
        {
            minCellZ = static_cast<int>((z - radius + mWorldSizeZ) / mCellSize);
            maxCellZ = static_cast<int>((z + radius + mWorldSizeZ) / mCellSize);
            minCellZ = (std::max)(minCellZ, 0);
            maxCellZ = (std::min)(maxCellZ, mGridSizeZ - 1);
        }

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
    }

    void Clear()
    {
        for (auto& cell : mCells)
        {
            cell.clear();
        }
    }

private:
    uint32_t PositionToCellIndex(float x, float y, float z) const
    {
        int cellX = static_cast<int>((x + mWorldSizeX) / mCellSize);
        int cellY = static_cast<int>((y + mWorldSizeY) / mCellSize);

        cellX = (std::max)(0, (std::min)(cellX, mGridSizeX - 1));
        cellY = (std::max)(0, (std::min)(cellY, mGridSizeY - 1));

        int cellZ = 0;
        if (Dim == 3)
        {
            cellZ = static_cast<int>((z + mWorldSizeZ) / mCellSize);
            cellZ = (std::max)(0, (std::min)(cellZ, mGridSizeZ - 1));
        }

        return cellX + cellY * mGridSizeX + cellZ * mGridSizeX * mGridSizeY;
    }

    std::vector<std::vector<uint32_t>> mCells;
    float mCellSize;
    float mWorldSizeX;
    float mWorldSizeY;
    float mWorldSizeZ;
    int mGridSizeX;
    int mGridSizeY;
    int mGridSizeZ;
};