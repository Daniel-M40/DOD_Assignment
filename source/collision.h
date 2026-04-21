// Collision detection functions

#ifndef MSC_COLLISION_H_INCLUDED
#define MSC_COLLISION_H_INCLUDED

#include "sprite.h"

namespace msc
{
	// Check a sprite position against the positions of all the sprites in an array of "blockers". If 
	// the sprite overlaps any blocker then it's position should be moved away so it no longer overlaps
	void BlockSprite(SpriteInstance& sprite, Vector2f spriteSize, Vector2f spriteAnchor,
	                 const SpriteInstance* blockers, uint32_t numBlockers, Vector2f blockerSize,
	                 Vector2f blockerAnchor);

	// Given an array of sprites, check each sprite position against the positions of all the sprites in
	// an array of "blockers". If any sprite overlaps any blocker then it's position should be moved away
	// so it no longer overlaps. Same as function above but with many sprites against many blockers
	void BlockSprites(SpriteInstance* sprites, uint32_t numSprites, Vector2f spriteSize, Vector2f spriteAnchor,
	                  const SpriteInstance* blockers, uint32_t numBlockers, Vector2f blockerSize,
	                  Vector2f blockerAnchor);
} // namespaces

#endif // Header guard
