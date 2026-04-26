#pragma once
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <cstring>
#include <atomic>
#include <memory>

template<int Dim>
class SpatialHash
{
public:
    SpatialHash(float cellSize, float worldSizeX, float worldSizeY, float worldSizeZ = 0.0f)
        : mCellSize(cellSize), mWorldSizeX(worldSizeX), mWorldSizeY(worldSizeY), mWorldSizeZ(worldSizeZ)
    {
        mInvCellSize = 1.0f / cellSize;
        mGridSizeX = static_cast<int>((worldSizeX * 2.0f) * mInvCellSize) + 1;
        mGridSizeY = static_cast<int>((worldSizeY * 2.0f) * mInvCellSize) + 1;
        mGridSizeZ = 1;

        if constexpr (Dim == 3)
        {
            mGridSizeZ = static_cast<int>((worldSizeZ * 2.0f) * mInvCellSize) + 1;
        }

        mNumCells = mGridSizeX * mGridSizeY * mGridSizeZ;
        mCounts.resize(mNumCells + 1, 0);
        mEntries.reserve(500000);

        // Allocate atomic arrays (can't use vector — atomics are non-copyable)
        mAtomicCounts = std::make_unique<std::atomic<uint32_t>[]>(mNumCells + 1);
        mAtomicWriteHeads = std::make_unique<std::atomic<uint32_t>[]>(mNumCells);
    }

    void Clear()
    {
        std::memset(mCounts.data(), 0, (mNumCells + 1) * sizeof(uint32_t));
        for (uint32_t i = 0; i < mNumCells + 1; ++i)
            mAtomicCounts[i].store(0, std::memory_order_relaxed);
        mInsertCount = 0;
    }

    // Single-threaded count
    void Count(float x, float y, float z)
    {
        uint32_t cell = PositionToCellIndex(x, y, z);
        mCounts[cell]++;
        mInsertCount++;
    }

    // Thread-safe count — for parallel spatial hash build
    void CountAtomic(float x, float y, float z)
    {
        uint32_t cell = PositionToCellIndex(x, y, z);
        mAtomicCounts[cell].fetch_add(1, std::memory_order_relaxed);
    }

    void BuildOffsets()
    {
        // Merge atomic counts into regular counts
        uint32_t totalCount = 0;
        for (uint32_t i = 0; i < mNumCells; ++i)
        {
            uint32_t c = mAtomicCounts[i].load(std::memory_order_relaxed);
            if (c == 0) c = mCounts[i];  // Fallback to serial counts if atomics weren't used
            mCounts[i] = c;
            totalCount += c;
        }

        mInsertCount = totalCount;
        mEntries.resize(mInsertCount);

        // Prefix sum
        uint32_t sum = 0;
        for (uint32_t i = 0; i < mNumCells; ++i)
        {
            uint32_t count = mCounts[i];
            mCounts[i] = sum;
            sum += count;
        }
        mCounts[mNumCells] = sum;

        // Set up write heads
        mWriteHeads.resize(mNumCells);
        std::memcpy(mWriteHeads.data(), mCounts.data(), mNumCells * sizeof(uint32_t));
        for (uint32_t i = 0; i < mNumCells; ++i)
            mAtomicWriteHeads[i].store(mCounts[i], std::memory_order_relaxed);
    }

    // Single-threaded insert
    void Insert(float x, float y, float z, uint32_t index)
    {
        uint32_t cell = PositionToCellIndex(x, y, z);
        mEntries[mWriteHeads[cell]++] = index;
    }

    // Thread-safe insert — for parallel spatial hash build
    void InsertAtomic(float x, float y, float z, uint32_t index)
    {
        uint32_t cell = PositionToCellIndex(x, y, z);
        uint32_t pos = mAtomicWriteHeads[cell].fetch_add(1, std::memory_order_relaxed);
        mEntries[pos] = index;
    }

    template<typename Func>
    void Query(float x, float y, float z, float radius, Func&& callback) const
    {
        int minCellX = static_cast<int>((x - radius + mWorldSizeX) * mInvCellSize);
        int maxCellX = static_cast<int>((x + radius + mWorldSizeX) * mInvCellSize);
        int minCellY = static_cast<int>((y - radius + mWorldSizeY) * mInvCellSize);
        int maxCellY = static_cast<int>((y + radius + mWorldSizeY) * mInvCellSize);

        minCellX = (std::max)(minCellX, 0);
        maxCellX = (std::min)(maxCellX, mGridSizeX - 1);
        minCellY = (std::max)(minCellY, 0);
        maxCellY = (std::min)(maxCellY, mGridSizeY - 1);

        int minCellZ = 0;
        int maxCellZ = 0;

        if constexpr (Dim == 3)
        {
            minCellZ = static_cast<int>((z - radius + mWorldSizeZ) * mInvCellSize);
            maxCellZ = static_cast<int>((z + radius + mWorldSizeZ) * mInvCellSize);
            minCellZ = (std::max)(minCellZ, 0);
            maxCellZ = (std::min)(maxCellZ, mGridSizeZ - 1);
        }

        for (int cz = minCellZ; cz <= maxCellZ; ++cz)
        {
            for (int cy = minCellY; cy <= maxCellY; ++cy)
            {
                for (int cx = minCellX; cx <= maxCellX; ++cx)
                {
                    uint32_t cellIdx = cx + cy * mGridSizeX + cz * mGridSizeX * mGridSizeY;
                    uint32_t start = mCounts[cellIdx];
                    uint32_t end = mCounts[cellIdx + 1];

                    for (uint32_t e = start; e < end; ++e)
                    {
                        callback(mEntries[e]);
                    }
                }
            }
        }
    }

    // Batch query: calls callback with (const uint32_t* indices, uint32_t count) per cell.
    // Exposes contiguous index ranges so callers can process in batches of 4+.
    template<typename BatchFunc>
    void QueryBatch(float x, float y, float z, float radius, BatchFunc&& callback) const
    {
        int minCellX = static_cast<int>((x - radius + mWorldSizeX) * mInvCellSize);
        int maxCellX = static_cast<int>((x + radius + mWorldSizeX) * mInvCellSize);
        int minCellY = static_cast<int>((y - radius + mWorldSizeY) * mInvCellSize);
        int maxCellY = static_cast<int>((y + radius + mWorldSizeY) * mInvCellSize);

        minCellX = (std::max)(minCellX, 0);
        maxCellX = (std::min)(maxCellX, mGridSizeX - 1);
        minCellY = (std::max)(minCellY, 0);
        maxCellY = (std::min)(maxCellY, mGridSizeY - 1);

        int minCellZ = 0;
        int maxCellZ = 0;

        if constexpr (Dim == 3)
        {
            minCellZ = static_cast<int>((z - radius + mWorldSizeZ) * mInvCellSize);
            maxCellZ = static_cast<int>((z + radius + mWorldSizeZ) * mInvCellSize);
            minCellZ = (std::max)(minCellZ, 0);
            maxCellZ = (std::min)(maxCellZ, mGridSizeZ - 1);
        }

        for (int cz = minCellZ; cz <= maxCellZ; ++cz)
        {
            for (int cy = minCellY; cy <= maxCellY; ++cy)
            {
                for (int cx = minCellX; cx <= maxCellX; ++cx)
                {
                    uint32_t cellIdx = cx + cy * mGridSizeX + cz * mGridSizeX * mGridSizeY;
                    uint32_t start = mCounts[cellIdx];
                    uint32_t end = mCounts[cellIdx + 1];

                    if (start < end)
                    {
                        callback(mEntries.data() + start, end - start);
                    }
                }
            }
        }
    }

private:
    uint32_t PositionToCellIndex(float x, float y, float z) const
    {
        int cellX = static_cast<int>((x + mWorldSizeX) * mInvCellSize);
        int cellY = static_cast<int>((y + mWorldSizeY) * mInvCellSize);

        cellX = (std::max)(0, (std::min)(cellX, mGridSizeX - 1));
        cellY = (std::max)(0, (std::min)(cellY, mGridSizeY - 1));

        int cellZ = 0;
        if constexpr (Dim == 3)
        {
            cellZ = static_cast<int>((z + mWorldSizeZ) * mInvCellSize);
            cellZ = (std::max)(0, (std::min)(cellZ, mGridSizeZ - 1));
        }

        return cellX + cellY * mGridSizeX + cellZ * mGridSizeX * mGridSizeY;
    }

    // Flat storage
    std::vector<uint32_t> mCounts;
    std::vector<uint32_t> mWriteHeads;
    std::vector<uint32_t> mEntries;

    // Atomic arrays for thread-safe parallel builds (unique_ptr because atomics are non-copyable)
    std::unique_ptr<std::atomic<uint32_t>[]> mAtomicCounts;
    std::unique_ptr<std::atomic<uint32_t>[]> mAtomicWriteHeads;

    uint32_t mInsertCount = 0;
    uint32_t mNumCells = 0;

    float mCellSize;
    float mInvCellSize;
    float mWorldSizeX;
    float mWorldSizeY;
    float mWorldSizeZ;
    int mGridSizeX;
    int mGridSizeY;
    int mGridSizeZ;
};