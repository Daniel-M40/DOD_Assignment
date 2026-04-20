// A sprite set is a collection sprites all held together on a single texture (a "sprite atlas")
// The class manages the preparation of an atlas and also manages the rendering of all sprite
// instances using sprites on this atlas. The primary motivation behind a sprite set is that a
// large number of *different* sprites can be rendered in a single draw call

#ifndef MSC_SPRITE_SET_H_INCLUDED
#define MSC_SPRITE_SET_H_INCLUDED

#include "sprite.h"
#include <d3d11.h>
#include <atlbase.h>
#include <vector>
#include <memory>
#include <stdint.h>

#pragma warning (push, 0)       // Cimg library emits distracting warnings so temporarily disable all warnings
#pragma warning (disable: 4702) // For some reason this warning (unreachable code) needs to be disabled seperately
#define cimg_use_jpeg  // jpeg support
#define cimg_use_png   // png support
#include <CImg.h>
using namespace cimg_library;
#undef HAVE_STDDEF_H // Get rid of warnings caused by jpeglib
#undef HAVE_STDLIB_H
#pragma warning (pop)

#include <Eigen/Core>
using Eigen::Vector2i;
using Eigen::Vector2f;
using Eigen::Vector3f;


namespace msc {

//-------------------------------------------------------------------------------------------------
// Types used by SpriteSet Class
//-------------------------------------------------------------------------------------------------

// The Sprite structure describes a sprite's dimensions, anchor and its location within
// a sprite atlas. The user doesn't work with Sprite structures directly, but instead gets
// SpriteIDs, which are used internally to look up this information when rendering.
// This should be a private structure in the class below, but leaving it near the related stuctures for ease of reading
struct Sprite
{
  Vector2f size;       // Dimensions of the sprite in pixels
  Vector2f anchor;     // Anchor position in pixels, with 0,0 being top left. This point is used for positioning/rotation
  Vector2f UV;         // UV for top left of sprite in the sprite atlas
  uint32_t atlasIndex; // Index into sprite atlas array (only needed for very large sprite sets, so usually 0)
  
  SpriteID ID;          // These two members are only used for laying out the sprites in the sprite atlas
  std::string filename; // --"--
};


// Just the dimensions part of the above structure, used when the gameplay needs this information
struct SpriteSize
{
  Vector2f size;       // Dimensions of the sprite in pixels
  Vector2f anchor;     // Anchor position in pixels, with 0,0 being top left. This point is used for positioning/rotation
};


// Data passed to the GPU to render each sprite - we will fill the vertex buffer with these.
// The first few members are taken directly from the sprite instance (see above), most of the
// other members come directly from the sprite structure (also see above)
// This should be a private structure in the class below, but leaving it near the related stuctures for ease of reading
//
__declspec(align(16)) // Align data to match HLSL packing rules - this structure must match exactly the shader version
struct SpriteRenderData
{
  Vector3f position; // XY is position on screen in pixels, with 0,0 at the top left. Z is depth order, 0->1
  float    rotation; // In radians, anticlockwise
  Vector2f scale;    // XY scaling, 1,1 by default
  uint32_t custom1;  // Padding: match HLSL rules; don't cross 16 byte boundaries; match the other sprite structures
  uint32_t custom2;  // Padding
  Vector2f size;     // The remaining members are described in the Sprite structure above
  Vector2f anchor;   // --"--
  Vector2f UV;
  uint32_t atlasIndex;
  uint32_t custom3;  // Padding
};


//-------------------------------------------------------------------------------------------------
// Class declaration
//-------------------------------------------------------------------------------------------------

class SpriteSet
{
  //-------------------------------------------------------------------------------------------------
  // Construction
  //-------------------------------------------------------------------------------------------------
public:
  SpriteSet(ID3D11Device& device);

  // Prevent copy/move/assignment
  SpriteSet(const SpriteSet&) = delete;
  SpriteSet(SpriteSet&&) = delete;
  SpriteSet& operator=(const SpriteSet&) = delete;
  SpriteSet& operator=(SpriteSet&&) = delete;


  //-------------------------------------------------------------------------------------------------
  // Sprite Set Preparation
  //-------------------------------------------------------------------------------------------------
public:
  // Add a sprite to a sprite set, also defining the anchor point. At this stage only basic sprite
  // data is collected. Calling the Finalise method will collate all the defined sprites and finalise
  // their structures. After Finalise is called this function will do nothing (sprite sets are fixed)
  // Returns the SpriteID with which to reference this sprite in other methods, or INVALID_ID on 
  // error (for example file error)
  SpriteID DefineSprite(std::string filename, SpriteAnchor anchor, float anchorX = 0.5f, float anchorY = 0.5f);

  // Call Finalise when all sprites have been defined on a sprite set. The method prepares an atlas
  // of the sprites on a large texture. Does not load the sprites ready for use yet, use the Load
  // method for that. This method can only be called once and afterward you cannot call DefineSprite
  // to add any more sprites (it will do nothing). Returns true on success
  bool Finalise();


  //-------------------------------------------------------------------------------------------------
  // Properties
  //-------------------------------------------------------------------------------------------------
public:
  Vector2i AtlasSize()  { return mAtlasSize; }

  const Sprite& GetSprite(SpriteID sprite) { return mSprites[sprite]; }

  //-------------------------------------------------------------------------------------------------
  // GPU Residency
  //-------------------------------------------------------------------------------------------------
public:
  // Bring the sprite set texture into GPU memory and prepare buffers for rendering sprite instances
  // Specify the maximum number of sprite instances from this set that will be rendered at once
  // Returns true on success
  bool Load(uint32_t maxInstances);

  // Release the sprite set texture from GPU memory and the sprite instance buffers
  void Unload();
  

  //-------------------------------------------------------------------------------------------------
  // Instances / Rendering
  //-------------------------------------------------------------------------------------------------
public:
  // Add a collection of sprite instances from this set to be rendered this frame. The user calls
  // this method once for each collection of instances they want drawn - how the user manages their
  // instances is not relevant to this function, only that the instance data is contiguous in the
  // 'instances' parameter. When all instance collections have been passed to this method, call the
  // Render method (see below) to render them all in one go
  // If The *total* number of instances passed to this method between calls to Render exceeds the
  // maxInstances value passed to the Load method then the extra instances will not be drawn
  // Optionally pass an x,y offset for all instances - useful for scrolling
  void AddToRenderQueue(SpriteInstance* instances, uint32_t number, Vector2f offset = {0, 0});

  // Render all the sprite instances that have been passed to the AddInstancesToRender function since
  // the last call to this function. Returns true on success
  bool Render();


 	//-------------------------------------------------------------------------------------------------
	// Support methods
 	//-------------------------------------------------------------------------------------------------
private:
  // Create a sprite atlas, a large texture that contains all the sprites in this set
  // Returns true on success
  bool CreateSpriteAtlas();


 	//-------------------------------------------------------------------------------------------------
	// Data
 	//-------------------------------------------------------------------------------------------------
private:
  // DirectX interfaces
  CComPtr<ID3D11Device>        mDevice;
  CComPtr<ID3D11DeviceContext> mContext;

  //---------------

  std::vector<Sprite> mSprites; // Details of all the sprites in this set, indexed by SpriteID.
                                // During certain processing this list is temporarily reordered.
                                // It must returned back to SpriteID order when done 

  // Total area of sprites in the set - used as a starting point for the sprite atlas size
  uint32_t mTotalSpriteArea = 0;
  
  Vector2i      mAtlasSize{}; // Dimensions of sprite atlas       
  CImg<uint8_t> mAtlasCPU;    // Sprite atlas in CPU memory
  CComPtr<ID3D11Texture2D>          mAtlasGPU; // Sprite atlas commited to GPU memory with Load method
  CComPtr<ID3D11ShaderResourceView> mAtlasSRV; // Interface to access GPU atlas in shaders

  bool mAtlasCreated = false;  // Atlas is ready in CPU memory
  bool mLoaded       = false;  // Atlas texture in GPU memory & sprite instance buffers are ready

  //---------------

  // An array to hold sprite instances to be rendered this frame. The array is a fixed maximum size, with
  // mInstancesEnd marking the current end of the array this frame. The array is emptied after rendering.
  // The class user manages their own instances and passes those to render over to this array each frame.
  // Note the use of a unique_ptr array
  std::unique_ptr<SpriteRenderData[]> mInstancesToRender;
  uint32_t                            mMaxInstances = 0;               // Overall size of above array (maximum capacity)
  SpriteRenderData*                   mInstancesToRenderEnd = nullptr; // Current end of the above array (indicates number
                                                                       // of sprites to render this frame)
  // GPU vertex buffer containing above data
  CComPtr<ID3D11Buffer>             mVertexBuffer;
};


} // namespaces

#endif // Header guard
