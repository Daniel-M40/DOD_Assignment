// Manage a set of sprite instances arranged in a grid with support for parallax scrolling

#include "scroll_plane.h"
#include <algorithm>
#include "utility.h"

namespace msc
{
	// Constructor creates a random collection of sprites, randomly chosen from the given vector. The sprites
	// are created in the area topLeft->bottomRight aligned to blocks of size gridSize. All have the given
	// scale and z value. Pass a multiplier for scrolling speed for parallax effects
	ScrollPlane::ScrollPlane(SpriteSet& spriteSet, Vector2i topLeft, Vector2i bottomRight, float z, uint32_t gridSize,
	                         float scale, float scrollSpeed, uint32_t numberBlocks, std::vector<SpriteID> sprites)
		: mSpriteSet(&spriteSet), mScrollSpeed(scrollSpeed)
	{
		mBlocks.resize(numberBlocks);

		uint32_t numSprites = static_cast<uint32_t>(sprites.size());
		for (auto& block : mBlocks)
		{
			// Get random position and round down to nearest grid size
			int32_t x = RandomInRange(topLeft.x(), bottomRight.x());
			int32_t y = RandomInRange(topLeft.y(), bottomRight.y());
			x &= ~(gridSize - 1);
			// Bitwise maths, & is bitwise and and ~ is bitwise not in C++. Assumes gridsize is a power of 2
			y &= ~(gridSize - 1);
			block.position.x() = static_cast<float>(x);
			block.position.y() = static_cast<float>(y);
			block.position.z() = z;
			block.scale = {scale, scale};
			block.rotation = 0;
			block.ID = sprites[RandomInRange(0, numSprites - 1)];
		}
	}


	// Add all the blocks to the render queue on the sprite set. Call the sprite set's
	// Render method to actually perform the rendering. Optionally pass an x,y offset
	// for all instances for scrolling - the offset will be scaled (for parallax effect)
	void ScrollPlane::AddToRenderQueue(Vector2f offset /*= {0, 0}*/)
	{
		mSpriteSet->AddToRenderQueue(mBlocks.data(), static_cast<uint32_t>(mBlocks.size()), offset * mScrollSpeed);
	}
} // namespaces
