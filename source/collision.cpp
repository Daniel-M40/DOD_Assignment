// Collision detection functions

#include "collision.h"
#include <cmath>

namespace msc
{
	//*******
	// The first version of the collision detection illustrates the algorithm. It is designed to do minimal
	// work in the inner loop for the common case of no collision. However, this version not optimised to
	// make the algorithm a little clearer. An optimised version is presented afterwards.
	//*******

	// ASCII Explanation (x-axis only, y-axis is treated the same way)
	//
	//            overlapLeft = how much the left sprite edge has gone "inside" the right blocker edge (+ve here)
	//     |<-----------------------|
	//     +---+                +---+
	//     | S |                | B |
	//     +---+                +---+
	//         |<---------------|
	//            overlapRight = how much the right sprite edge has gone inside the left blocker edge (-ve here)
	//
	// RESULT: one of the overlap values is negative, so no collision
	//
	//
	//          overlapLeft = still +ve, the sprite left edge is still "inside" the blocker right edge
	//     |<-----------------|
	//     +----------+
	//     |      +---|-------+
	//     |    S |   |       |
	//     |      |   | B     |
	//     +------|---+       |
	//            +-----------+
	//            |-->|
	//         overlapRight = +ve now, the right sprite edge has gone "inside" the blocker left edge
	//
	// RESULT: Because all overlaps are +ve there must be a collision. The smallest overlap indicates 
	//         the direction of the smallest movement for the sprite to "unembed" itself.


	// Check a sprite position against the positions of all the sprites in an array of "blockers". If 
	// the sprite overlaps any blocker then it's position is moved away so it no longer overlaps.
	// Pass the sprite size and anchor position, and the blocker size and anchor position.
	// Assumes all blockers are the same size and anchor. Takes account of sprite and blocker scale,
	// but assumes all blockers have the same scale. Ignores rotation.
	//
	// Note the choices I made to get the sprite sizes and anchors to this function. Fairly flexible, 
	// but does mean all blocking sprites must be the same size
	void BlockSpriteX(SpriteInstance& sprite, Vector2f spriteSize, Vector2f spriteAnchor,
	                  const SpriteInstance* blockers, uint32_t numBlockers, Vector2f blockerSize,
	                  Vector2f blockerAnchor)
	{
		// Precalculate some sprite coordinates
		float sx = sprite.position.x();
		float sy = sprite.position.y();
		float sl = sx - spriteAnchor.x() * sprite.scale.x(); // X coordinate of left edge of sprite
		float st = sy - spriteAnchor.y() * sprite.scale.y(); // Y coordinate of top edge of sprite
		float sr = sx + (spriteSize.x() - spriteAnchor.x()) * sprite.scale.x(); // X coordinate of right edge of sprite
		float sb = sy + (spriteSize.y() - spriteAnchor.y()) * sprite.scale.y(); // Y coordinate of bottom edge of sprite

		// Assuming all blockers have same size and anchor so we can do some precalculation
		float blo = -blockerAnchor.x() * blockers->scale.x();
		// X offset to left edge of blocker (add to blocker position when known)
		float bto = -blockerAnchor.y() * blockers->scale.y(); // Y offset to top edge of blocker
		float bro = (blockerSize.x() - blockerAnchor.x()) * blockers->scale.x(); // X offset to right edge of blocker
		float bbo = (blockerSize.y() - blockerAnchor.y()) * blockers->scale.y(); // Y offset to bottom edge of blocker

		auto blockersEnd = blockers + numBlockers;
		while (blockers != blockersEnd)
		{
			float bx = blockers->position.x();
			float by = blockers->position.y();
			float bl = bx + blo; // X coordinate of left edge of blocker
			float bt = by + bto; // Y coordinate of top edge of blocker
			float br = bx + bro; // X coordinate of left edge of blocker
			float bb = by + bbo; // Y coordinate of bottom edge of blocker

			float overlapRight = sr - bl;
			// How much the right edge of the sprite overlaps the left edge of the blocker, -ve means no collision
			float overlapLeft = br - sl; // Same with other edges
			float overlapBottom = sb - bt;
			float overlapTop = bb - st;

			// If any of the above tests are negative then no collision. i.e. all of them must be positive for a collision
			if (overlapLeft > 0 && overlapRight > 0 && overlapTop > 0 && overlapBottom > 0)
			{
				// Find the smallest overlap and move the sprite away in that direction
				float overlapX = overlapLeft < overlapRight ? overlapLeft : -overlapRight;
				float overlapY = overlapTop < overlapBottom ? overlapTop : -overlapBottom;
				if (abs(overlapX) < abs(overlapY))
				{
					sprite.position.x() += overlapX;
				}
				else
				{
					sprite.position.y() += overlapY;
				}

				// Calculate the new sprite edge coordinates
				sx = sprite.position.x();
				sy = sprite.position.y();
				sl = sx - spriteAnchor.x() * sprite.scale.x();
				st = sy - spriteAnchor.y() * sprite.scale.y();
				sr = sx + (spriteSize.x() - spriteAnchor.x()) * sprite.scale.x();
				sb = sy + (spriteSize.y() - spriteAnchor.y()) * sprite.scale.y();
			}
			++blockers;
		}
	}


	//-------------------------------------------------------------------------------------------------

	// Optimised version of above
	// Note how minimal the inner loop is. If no collision (most common case): 2 memory accesses, 4 add/subtract, 4 comparisons and the loop
	void BlockSprite(SpriteInstance& sprite, Vector2f spriteSize, Vector2f spriteAnchor,
	                 const SpriteInstance* blockers, uint32_t numBlockers, Vector2f blockerSize, Vector2f blockerAnchor)
	{
		// Precalculate some sprite coordinates
		float sx = sprite.position.x();
		float sy = sprite.position.y();
		float sl = sx - spriteAnchor.x() * sprite.scale.x(); // X coordinate of left edge of sprite
		float st = sy - spriteAnchor.y() * sprite.scale.y(); // Y coordinate of top edge of sprite
		float sr = sx + (spriteSize.x() - spriteAnchor.x()) * sprite.scale.x(); // X coordinate of right edge of sprite
		float sb = sy + (spriteSize.y() - spriteAnchor.y()) * sprite.scale.y(); // Y coordinate of bottom edge of sprite

		// Assuming all blockers have same size and anchor so we can do some precalculation
		float blo = -blockerAnchor.x() * blockers->scale.x();
		// X offset to left edge of blocker (add to blocker position when known)
		float bto = -blockerAnchor.y() * blockers->scale.y(); // Y offset to top edge of blocker
		float bro = (blockerSize.x() - blockerAnchor.x()) * blockers->scale.x(); // X offset to right edge of blocker
		float bbo = (blockerSize.y() - blockerAnchor.y()) * blockers->scale.y(); // Y offset to bottom edge of blocker

		// Extract more fixed calculation out of the inner loop
		float sr_blo = sr - blo;
		float sl_bro = sl - bro;
		float sb_bto = sb - bto;
		float st_bbo = st - bbo;

		auto blockersEnd = blockers + numBlockers;
		while (blockers != blockersEnd)
		{
			// Calulate overlaps
			float bx = blockers->position.x();
			float by = blockers->position.y();
			float overlapRight = sr_blo - bx;
			float overlapLeft = bx - sl_bro;
			float overlapBottom = sb_bto - by;
			float overlapTop = by - st_bbo;
			if (overlapLeft > 0 && overlapRight > 0 && overlapTop > 0 && overlapBottom > 0)
			{
				// Find the smallest overlap and move the sprite away in that direction. Recalculate only the necessary sprite coordinate values
				float overlapX = overlapLeft < overlapRight ? overlapLeft : -overlapRight;
				float overlapY = overlapTop < overlapBottom ? overlapTop : -overlapBottom;
				if (abs(overlapX) < abs(overlapY))
				{
					sprite.position.x() += overlapX, sl_bro += overlapX, sr_blo += overlapX;
				}
				else
				{
					sprite.position.y() += overlapY, sb_bto += overlapY, st_bbo += overlapY;
				}
			}
			++blockers;
		}
	}


	//-------------------------------------------------------------------------------------------------

	// Given an array of sprites, check each sprite position against the positions of all the sprites in
	// an array of "blockers". If any sprite overlaps any blocker then it's position should be moved away
	// so it no longer overlaps. Same as function above but with many sprites against many blockers
	void BlockSprites(SpriteInstance* sprites, uint32_t numSprites, Vector2f spriteSize, Vector2f spriteAnchor,
	                  const SpriteInstance* blockers, uint32_t numBlockers, Vector2f blockerSize,
	                  Vector2f blockerAnchor)
	{
		// Assuming all blockers have same size and anchor so we can do some precalculation
		float blo = -blockerAnchor.x() * blockers->scale.x();
		// X offset to left edge of blocker (add to blocker position when known)
		float bto = -blockerAnchor.y() * blockers->scale.y(); // Y offset to top edge of blocker
		float bro = (blockerSize.x() - blockerAnchor.x()) * blockers->scale.x(); // X offset to right edge of blocker
		float bbo = (blockerSize.y() - blockerAnchor.y()) * blockers->scale.y(); // Y offset to bottom edge of blocker

		auto spritesEnd = sprites + numSprites;
		auto blockersEnd = blockers + numBlockers;
		while (sprites != spritesEnd)
		{
			// Precalculate some sprite coordinates
			float sx = sprites->position.x();
			float sy = sprites->position.y();
			float sl = sx - spriteAnchor.x() * sprites->scale.x(); // X coordinate of left edge of sprite
			float st = sy - spriteAnchor.y() * sprites->scale.y(); // Y coordinate of top edge of sprite
			float sr = sx + (spriteSize.x() - spriteAnchor.x()) * sprites->scale.x();
			// X coordinate of right edge of sprite
			float sb = sy + (spriteSize.y() - spriteAnchor.y()) * sprites->scale.y();
			// Y coordinate of bottom edge of sprite

			// Extract more fixed calculation out of the inner loop
			float sr_blo = sr - blo;
			float sl_bro = sl - bro;
			float sb_bto = sb - bto;
			float st_bbo = st - bbo;

			const SpriteInstance* blocker = blockers;
			while (blocker != blockersEnd)
			{
				float bx = blocker->position.x();
				float by = blocker->position.y();
				float overlapRight = sr_blo - bx;
				float overlapLeft = bx - sl_bro;
				float overlapBottom = sb_bto - by;
				float overlapTop = by - st_bbo;

				if (overlapLeft * overlapRight > 0 && overlapTop * overlapBottom > 0)
				{
					// Find the smallest overlap and move the sprite away in that direction. Recalculate the sprite coordinate values
					float overlapX = overlapLeft < overlapRight ? overlapLeft : -overlapRight;
					float overlapY = overlapTop < overlapBottom ? overlapTop : -overlapBottom;
					if (abs(overlapX) < abs(overlapY))
					{
						sprites->position.x() += overlapX, sl_bro += overlapX, sr_blo += overlapX;
					}
					else
					{
						sprites->position.y() += overlapY, sb_bto += overlapY, st_bbo += overlapY;
					}
				}
				++blocker;
			}
			++sprites;
		}
	}
} // namespaces
