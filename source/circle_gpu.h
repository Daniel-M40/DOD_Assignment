#pragma once

struct alignas(16) CircleGPU
{
	float posX, posY, posZ;
	float radius;
	float colR, colG, colB;
	float padding;
};
