#pragma once


#define VISUALISATION_ENABLED

class SimConfig
{
public:
    static const int NUM_CIRCLES = 100000;
    static const int WORLD_SIZE_X = 1600;
    static const int WORLD_SIZE_Y = 960;
    constexpr static float MAX_VELOCITY = 100.0f;
    constexpr static float RADIUS = 5.f;
};