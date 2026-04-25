#pragma once

struct alignas(16) CircleGPU
{
	float posX, posY, posZ;
	float radius;
	float colR, colG, colB;
	float padding;
};


struct QuadVertex
{
    float x, y; // -1 to 1, the unit quad corners
};

