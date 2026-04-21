// Miscellaneous support functions

#ifndef MSC_UTILITY_H_INCLUDED
#define MSC_UTILITY_H_INCLUDED

#include <stdint.h>

namespace msc
{
	// Return a float in the range [low, high]
	float RandomInRange(float low, float high);

	// Return a int in the range [low, high]. Size of range must be less than RAND_MAX
	int32_t RandomInRange(int32_t low, int32_t high);

	// Given a random event that occurs on average every "avgTime" seconds, return a random time
	// until the next event
	float RandomTime(float avgTime);

	// Conversions
	float ToRadians(float degrees);
	float ToDegrees(float radians);
} // namespaces

#endif // Header guard
