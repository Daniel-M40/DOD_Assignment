// Manage a set of sprite instances arranged in a grid with support for parallax scrolling

#ifndef MSC_SCROLL_PLANE_H_INCLUDED
#define MSC_SCROLL_PLANE_H_INCLUDED

#include "sprite_set.h"
#include <Eigen/Core>
using Eigen::Vector2i;

namespace msc {

class ScrollPlane
{
 	//-------------------------------------------------------------------------------------------------
	// Construction
 	//-------------------------------------------------------------------------------------------------
public:
  // Constructor creates a random collection of sprites, randomly chosen from the given vector. The sprites
  // are created in the area topLeft->bottomRight aligned to blocks of size gridSize. All have the given
  // scale and z value. Pass a multiplier for scrolling speed for parallax effects
  ScrollPlane(SpriteSet& spriteSet, Vector2i topLeft, Vector2i bottomRight, float z, uint32_t gridSize, 
              float scale, float scrollSpeed, uint32_t numberBlocks, std::vector<SpriteID> sprites);

  // Prevent copy/move/assignment
  ScrollPlane(const ScrollPlane&) = delete;
  ScrollPlane(ScrollPlane&&) = delete;
  ScrollPlane& operator=(const ScrollPlane&) = delete;
  ScrollPlane& operator=(ScrollPlane&&) = delete;


 	//-------------------------------------------------------------------------------------------------
	// Usage
 	//-------------------------------------------------------------------------------------------------
public:
  // Add all the blocks to the render queue on the sprite set. Call the sprite set's
  // Render method to actually perform the rendering. Optionally pass an x,y offset
  // for all instances for scrolling - the offset will be scaled (for parallax effect)
  void AddToRenderQueue(Vector2f offset = {0, 0});


 	//-------------------------------------------------------------------------------------------------
	// Data
 	//-------------------------------------------------------------------------------------------------
public:

  SpriteSet* mSpriteSet;

  std::vector<SpriteInstance> mBlocks;

  // Any offset passed for rendering are scaled by this value allowing simple parallax scrolling
  float mScrollSpeed;
};


} // namespaces

#endif // Header guard
