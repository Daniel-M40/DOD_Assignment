// Manage a set of sprite instances for a typical gravity-only particle system

#ifndef MSC_PARTICLE_SYSTEM_H_INCLUDED
#define MSC_PARTICLE_SYSTEM_H_INCLUDED

#include <stdint.h>
#include <vector>
#include "sprite_set.h"

#include <Eigen/Core>
#include <Eigen/Geometry>
using Eigen::Vector2f;
using Eigen::Rotation2Df;

namespace msc
{
	// Where is the best place to put this declaration? What if gravity is variable by the game play?
	const Vector2f GRAVITY{0, 250.0f};


	class ParticleSystem
	{
		//-------------------------------------------------------------------------------------------------
		// Construction
		//-------------------------------------------------------------------------------------------------
	public:
		ParticleSystem(SpriteSet& spriteSet, uint32_t maxParticles);

		// Prevent copy/move/assignment
		ParticleSystem(const ParticleSystem&) = delete;
		ParticleSystem(ParticleSystem&&) = delete;
		ParticleSystem& operator=(const ParticleSystem&) = delete;
		ParticleSystem& operator=(ParticleSystem&&) = delete;


		//-------------------------------------------------------------------------------------------------
		// Usage
		//-------------------------------------------------------------------------------------------------
	public:
		// Add a set of particles of the same type but with random variations in positioning
		// 'position' is the centre of area where the particles appear, 'positionSpread' the range around
		// this point in which they will be randomly placed. The particle scale will be a random value from
		// minScale to maxScale. The particle initial velocities will be centred on the 'velocity' vector
		// with a random angle up to angleSpread (radians) either side. The speed will be from minSpeed to
		// maxSpeed. Life is also randomised between minLife and maxLife.
		void AddParticles(Vector2f position, Vector2f positionSpread, float minScale, float maxScale,
		                  Vector2f velocity, float angleSpread, float minSpeed, float maxSpeed,
		                  float minLife, float maxLife, SpriteID spriteID, uint32_t numParticles);

		// Call to update particle positions. Standard Euler method with gravity as the only acceleration
		bool Update(float frameTime);

		// Add the current live particles to the render queue on the sprite set. Call the sprite set's
		// Render method to actually perform the rendering.
		// Optionally pass an x,y offset for all instances - useful for scrolling
		void AddToRenderQueue(Vector2f offset = {0, 0});


		//-------------------------------------------------------------------------------------------------
		// Data
		//-------------------------------------------------------------------------------------------------
	public:
		SpriteSet* mSpriteSet;
		uint32_t mMaxParticles;

		// Important point here: There are two vectors here, but they refer to the same "objects". For
		// example, particle 5 has its position and sprite ID in mSpriteInstances[5] and its velocity and
		// life in mUpdateData[5]. Understanding why is an exercise - this splitting of data across different
		// arrays is a very typical pattern in data-oriented-design
		struct UpdateData
		{
			Vector2f velocity;
			float life;
		};

		std::vector<UpdateData> mUpdateData;
		std::vector<SpriteInstance> mSpriteInstances;
	};
} // namespaces

#endif // Header guard
