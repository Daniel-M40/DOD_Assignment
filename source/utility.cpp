// Miscellaneous support functions

#include "utility.h"
#include <stdlib.h>

#define _USE_MATH_DEFINES  // Get the M_PI constant (amongst others)
#include <math.h>

namespace msc
{
	// Return a float in the range [low, high]
	float RandomInRange(float low, float high)
	{
		float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
		float diff = high - low;
		float r = random * diff;
		return low + r;
	}

	// Return a int in the range [low, high]. Size of range must be less than RAND_MAX
	int32_t RandomInRange(int32_t low, int32_t high)
	{
		uint32_t diff = high - low + 1;
		uint32_t random = rand() % diff;
		return low + random;
	}

	// Given a random event that occurs on average every "avgTime" seconds, return a random time
	// until the next event
	// See http://preshing.com/20111007/how-to-generate-random-timings-for-a-poisson-process/ (this function uses average time parameter which is 1/rate they use on the site)
	float RandomTime(float avgTime)
	{
		float r = static_cast<float>(rand()) / (RAND_MAX + 1);
		return -logf(1.0f - r) * avgTime;
	}

	// Conversions
	float ToRadians(float degrees)
	{
		return (static_cast<float>(M_PI) / 180) * degrees;
	}

	float ToDegrees(float radians)
	{
		return radians * (180 / static_cast<float>(M_PI));
	}
} // namespaces
