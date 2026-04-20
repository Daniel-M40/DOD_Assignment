// Manage a set of sprite instances for a typical gravity-only particle system

#include "particle_system.h"
#include "utility.h"

namespace msc {

//-------------------------------------------------------------------------------------------------
// Construction
//-------------------------------------------------------------------------------------------------

ParticleSystem::ParticleSystem(SpriteSet& spriteSet, uint32_t maxParticles) 
  : mSpriteSet(&spriteSet), mMaxParticles(maxParticles)
{
  mUpdateData.reserve(maxParticles);
  mSpriteInstances.reserve(maxParticles);
}


//-------------------------------------------------------------------------------------------------
// Usage
//-------------------------------------------------------------------------------------------------

// Add a set of particles of the same type but with random variations in positioning
// 'position' is the centre of area where the particles appear, 'positionSpread' the range around
// this point in which they will be randomly placed. The particle scale will be a random value from
// minScale to maxScale. The particle initial velocities will be centred on the 'velocity' vector
// with a random angle up to angleSpread (radians) either side. The speed will be from minSpeed to
// maxSpeed. Life is also randomised between minLife and maxLife.
void ParticleSystem::AddParticles(Vector2f position, Vector2f positionSpread, float minScale, float maxScale, 
                                  Vector2f velocity, float angleSpread, float minSpeed, float maxSpeed,
                                  float minLife, float maxLife, SpriteID spriteID, uint32_t numParticles)
{
  // Guard against adding too many particles
  if (mSpriteInstances.size() + numParticles > mMaxParticles)  
    numParticles = mMaxParticles - static_cast<uint32_t>(mSpriteInstances.size());  // Will ignore excess particles

  // Resize arrays
  uint32_t newSize = static_cast<uint32_t>(mSpriteInstances.size()) + numParticles;
  mSpriteInstances.resize(newSize);
  mUpdateData     .resize(newSize);

  // Loop through rendering data
  auto spriteInstance    = mSpriteInstances.data();
  auto spriteInstanceEnd = spriteInstance + numParticles;
  while (spriteInstance != spriteInstanceEnd)
  {
    // Set intial positioning and sprite ID
    spriteInstance->position.x() = position.x() + RandomInRange(-positionSpread.x(), positionSpread.x());
    spriteInstance->position.y() = position.y() + RandomInRange(-positionSpread.y(), positionSpread.y());
    spriteInstance->position.z() = 0.5f;
    spriteInstance->scale.x() = spriteInstance->scale.y() = RandomInRange(minScale, maxScale);
    spriteInstance->rotation = 0;
    spriteInstance->ID = spriteID;
    ++spriteInstance;
  }

  // Loop through update data
  auto updateData        = mUpdateData.data();
  auto updateDataEnd     = updateData + numParticles;
  while (updateData != updateDataEnd)
  {
    // Randomise velocity by speed and angle - a little maths
    float speed    = RandomInRange(minSpeed, maxSpeed);
    float rotation = RandomInRange(-angleSpread, angleSpread);
    updateData->velocity = speed * (Rotation2Df(rotation) * velocity.normalized());

    // Also randomise life
    updateData->life = RandomInRange(minLife, maxLife); 
    ++updateData;
  }
}


// Call to update particle positions. Standard Euler method with gravity as the only acceleration
bool ParticleSystem::Update(float frameTime)
{
  // Want scale to half every second. This is the correct way to handle frame time when you want
  // to multiply by something every second. However, hoist the pow calculation out of the loop.
  float scaleFactor = pow(0.5f, frameTime);

  // Get beginning and ends of particle data arrays
  uint32_t numParticles = static_cast<uint32_t>(mSpriteInstances.size());
  auto updateData        = mUpdateData.data();
  auto updateDataEnd     = updateData + numParticles;
  auto spriteInstance    = mSpriteInstances.data();
  auto spriteInstanceEnd = spriteInstance + numParticles;
    
  // Go through each particle. There are two corresponding collections of data, walk both at the same time.
  while (spriteInstance != spriteInstanceEnd)
  {
    // Particle update is standard Euler's method
    spriteInstance->position.head<2>() += updateData->velocity * frameTime;  // the head<2> accesses the x & y of a Vector3f as a Vector2f
    updateData->velocity += GRAVITY * frameTime;

    spriteInstance->scale *= scaleFactor;

    // If life reaches 0 efficiently delete particle
    updateData->life -= frameTime;
    if (updateData->life <= 0)
    {
	    // Copy the particle at the end of the collection over the current (dead) particle
      // Do this for each collection of data. Don't step the current pointer forward.
      --updateDataEnd;
      *updateData = *updateDataEnd;
      --spriteInstanceEnd;
      *spriteInstance = *spriteInstanceEnd;
    }
    else // Life didn't reach 0, just step to next particle
    {
      ++updateData;
      ++spriteInstance;
    }
  }

  // Resize arrays after any deletions
  uint32_t newSize = static_cast<uint32_t>(spriteInstanceEnd - mSpriteInstances.data());
  mSpriteInstances.resize(newSize);
  mUpdateData     .resize(newSize);

  // Returns false when no particles are left
  return mSpriteInstances.size() > 0;
}


// Add the currently live particles to the render queue on the sprite set. Call the sprite set's
// Render method to actually perform the rendering.
// Optionally pass an x,y offset for all instances - useful for scrolling
void ParticleSystem::AddToRenderQueue(Vector2f offset /*= {0, 0}*/)
{
  mSpriteSet->AddToRenderQueue(mSpriteInstances.data(), static_cast<uint32_t>(mSpriteInstances.size()), offset);
}


} // namespaces
