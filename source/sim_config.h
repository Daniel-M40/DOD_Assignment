#pragma once


#define VISUALISATION_ENABLED

class SimConfig
{
public:

    //---------------------------------
    // WORLD CONFIG
    //---------------------------------
	static const int NUM_CIRCLES = 100000;
    static const int WORLD_SIZE_X = 1600;
    static const int WORLD_SIZE_Y = 960;

    //---------------------------------
    // CIRCLE CONFIG
    //---------------------------------
	constexpr static float CIRCLE_MAX_VELOCITY = 100.0f;
    constexpr static float CIRCLE_RADIUS = 5.f;

    //---------------------------------
    // NODE CONFIG
    //---------------------------------
    constexpr static float NODE_MIN_RADIUS = 10.f;
    constexpr static float NODE_MAX_RADIUS = 60.f;
    constexpr static float NODE_MIN_TIME = 1.f;
    constexpr static float NODE_MAX_TIME = 5.f;

    constexpr static float ATTRACTOR_NODE_STRENGTH = 5.f;
    constexpr static float IMPLUSE_NODE_STRENGTH = 5.f;

    constexpr static bool MOVING_NODES = false;
};