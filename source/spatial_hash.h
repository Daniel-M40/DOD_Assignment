#pragma once
#include <functional>
#include <vector>
#include <stdint.h>
#include "sim_config.h"

class SpatialHash
{
public:
#ifdef ENABLE_3D
    SpatialHash(float cellSize, float worldSizeX, float worldSizeY, float worldSizeZ);
#else
    SpatialHash(float cellSize, float worldSizeX, float worldSizeY);
#endif

    void Insert(float x, float y,
#ifdef ENABLE_3D
        float z,
#endif
        uint32_t circleIndex);

    void Query(float x, float y,
#ifdef ENABLE_3D
        float z,
#endif
        float radius, std::function<void(uint32_t)> callback);

    void Clear();

private:
    uint32_t PositionToCellIndex(float x, float y
#ifdef ENABLE_3D
        , float z
#endif
    ) const;

    std::vector<std::vector<uint32_t>> mCells;
    float mCellSize;
    float mWorldSizeX;
    float mWorldSizeY;
    int mGridSizeX;
    int mGridSizeY;

#ifdef ENABLE_3D
    float mWorldSizeZ;
    int mGridSizeZ;
#endif
};