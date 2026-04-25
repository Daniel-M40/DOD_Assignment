#pragma once
#define VISUALISATION_ENABLED
#define THREAD_POOL_ENABLED
#define SPATIAL_HASH_ENABLED
//#define ENABLE_3D

class SimConfig
{
public:
    //---------------------------------
    // WORLD CONFIG
    //---------------------------------
    static const int NUM_CIRCLES = 2500000;
    static const int WORLD_SIZE_X = 1000;
    static const int WORLD_SIZE_Y = 1000;
    static const int WORLD_SIZE_Z = 1000;
    constexpr static bool ENABLE_WALLS = true;

    //---------------------------------
    // SPATIAL HASH CONFIG
    //---------------------------------
    static const int CELL_SIZE = 30;

    //---------------------------------
    // CIRCLE CONFIG
    //---------------------------------
    constexpr static float CIRCLE_MAX_VELOCITY = 5.0f;
    constexpr static float CIRCLE_RADIUS = 1.f;
    constexpr static float CIRCLE_MAX_HEALTH = 100.f;
    constexpr static bool CIRCLE_DEATH_ENABLED = false;
    constexpr static bool CIRCLE_COLLISION_ENABLED = false;

    //---------------------------------
    // NODE CONFIG
    //---------------------------------
    constexpr static float NODE_MIN_RADIUS = 10.f;
    constexpr static float NODE_MAX_RADIUS = 60.f;
    constexpr static float NODE_MIN_TIME = 1.f;
    constexpr static float NODE_MAX_TIME = 5.f;
    constexpr static float IMPULSE_NODE_STRENGTH = 50.f;
    constexpr static bool MOVING_NODES = false;
    constexpr static bool NODE_VISUALISATION_ENABLED = false;
};