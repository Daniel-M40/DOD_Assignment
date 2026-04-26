#pragma once

//#define ENABLE_3D

class SimConfig
{
public:
    //---------------------------------
    // BUILD FEATURES
    //---------------------------------
    static constexpr bool VISUALISATION = true;
    static constexpr bool THREADING = true;
    static constexpr bool SPATIAL_HASHING = true;

    //---------------------------------
    // WORLD CONFIG
    //---------------------------------
    static const int NUM_CIRCLES = 500000;
    static const int WORLD_SIZE_X = 1000;
    static const int WORLD_SIZE_Y = 1000;
    static const int WORLD_SIZE_Z = 1000;
    static constexpr bool ENABLE_WALLS = true;

    //---------------------------------
    // SPATIAL HASH CONFIG
    //---------------------------------
    static const int CELL_SIZE = 60;

    //---------------------------------
    // CIRCLE CONFIG
    //---------------------------------
    static constexpr float CIRCLE_MAX_VELOCITY = 5.0f;
    static constexpr float CIRCLE_RADIUS = 1.f;
    static constexpr float CIRCLE_MAX_HEALTH = 100.f;
    static constexpr bool CIRCLE_DEATH_ENABLED = false;
    static constexpr bool CIRCLE_COLLISION_ENABLED = true;

    //---------------------------------
    // NODE CONFIG
    //---------------------------------
    static constexpr float NODE_MIN_RADIUS = 10.f;
    static constexpr float NODE_MAX_RADIUS = 60.f;
    static constexpr float NODE_MIN_TIME = 1.f;
    static constexpr float NODE_MAX_TIME = 5.f;
    static constexpr float IMPULSE_NODE_STRENGTH = 50.f;
    static constexpr bool MOVING_NODES = true;
    static constexpr bool NODE_VISUALISATION_ENABLED = false;
};