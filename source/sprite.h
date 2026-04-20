// Core sprite definitions

#ifndef MSC_SPRITE_H_INCLUDED
#define MSC_SPRITE_H_INCLUDED

#include <stdint.h>
#include <Eigen/Core>
using Eigen::Vector2f;
using Eigen::Vector3f;

namespace msc {

// Sprite UIDs are simple integers
typedef uint32_t SpriteID;
const uint32_t INVALID_ID = 0xffffffff;

// The anchor is the point within the sprite by which it is positioned. It will also rotate around
// this point. A custom anchor allows an exact position to be passed when defining a sprite
enum class SpriteAnchor
{
  Centre,
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
  Custom
};


// XY is position on screen in pixels, with 0,0 at the top left. Z is depth order, 0->1
Vector3f SpritePosition[];

// Instance of a sprite to render - the sprite's ID plus its current position/rotation
// Used by the game logic to control the sprite. Passed to sprite set when rendering it.
struct SpriteInstance
{
  Vector3f position; 
  float    rotation; // In radians, anticlockwise
  Vector2f scale;    // XY scaling, 1,1 by default
  uint32_t custom1;  // Padding to match the other sprite structures
  SpriteID ID;       // ID of the sprite to render at this location
};


} // namespaces

#endif // Header guard
