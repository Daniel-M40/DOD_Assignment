#pragma once

class SimConfig
{
public:
	int numCircles = 100000;
	Vector2i worldSize = {1600, 960};
	Vector2f maxVelocity = {100.0f, 100.0f};
	float radius = 5.f;
};
