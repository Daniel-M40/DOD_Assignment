#pragma once
#include <functional>
#include <vector>
#include <stdint.h>

class SpatialHash
{
public:
    SpatialHash(float cellSize, float worldSizeX, float worldSizeY);

    void Insert(float x, float y, uint32_t circleIndex);
    void Query(float x, float y, float radius, std::function<void(uint32_t)> callback);
    void Clear();

private:
    uint32_t PositionToCellIndex(float x, float y) const;

    std::vector<std::vector<uint32_t>> mCells;
    float mCellSize;
    float mWorldSizeX;
    float mWorldSizeY;
    int mGridSizeX;
    int mGridSizeY;
};