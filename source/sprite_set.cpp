// A sprite set is a collection sprites all held together on a single texture (a "sprite atlas")
// The class manages the preparation of an atlas and also manages the rendering of all sprite
// instances using sprites on this atlas. The primary motivation behind a sprite set is that a
// large number of *different* sprites can be rendered in a single draw call

#include "sprite_set.h"
#include <algorithm>

namespace msc {

//-------------------------------------------------------------------------------------------------
// Construction
//-------------------------------------------------------------------------------------------------

SpriteSet::SpriteSet(ID3D11Device& device) : mDevice(&device)
{
  mDevice->GetImmediateContext(&mContext);
}


//-------------------------------------------------------------------------------------------------
// Sprite Set Preparation
//-------------------------------------------------------------------------------------------------

// Add a sprite to a sprite set, also defining the anchor point. At this stage only basic sprite
// data is collected. Calling the Finalise method will collate all the defined sprites and finalise
// their structures. After Finalise is called this function will do nothing (sprite sets are fixed)
// Returns the SpriteID with which to reference this sprite in other methods, or INVALID_ID on 
// error (for example file error)
SpriteID SpriteSet::DefineSprite(std::string filename, SpriteAnchor anchor, float anchorX /*= 0.5f*/, float anchorY /*= 0.5f*/)
{
  if (mAtlasCreated)  return INVALID_ID;  // Can't add more sprites after Finalise has been called

  try
  {
    // Attempt to load image, throws exception on failure - see below
    CImg<uint8_t> sprite(filename.c_str());

    // Add new sprite details to the sprite list. Actual pixel data is ignored & discarded at this stage
    uint32_t width  = sprite.width ();
    uint32_t height = sprite.height();
    Sprite newSprite{};  // With this syntax all values in Sprite will be 0 initialised
    newSprite.size = {width, height};
    if      (anchor == SpriteAnchor::Centre     )  newSprite.anchor = {width * 0.5f, height * 0.5f};
    else if (anchor == SpriteAnchor::TopLeft    )  newSprite.anchor = {      0,       0};
    else if (anchor == SpriteAnchor::TopRight   )  newSprite.anchor = {  width,       0};
    else if (anchor == SpriteAnchor::BottomLeft )  newSprite.anchor = {      0,  height};
    else if (anchor == SpriteAnchor::BottomRight)  newSprite.anchor = {  width,  height};
    else                                           newSprite.anchor = {anchorX, anchorY};

    newSprite.filename = filename;                         // Not loading file data until the Finalise method, so store filename now
    newSprite.ID = static_cast<uint32_t>(mSprites.size()); // The new sprite's ID refers to end of current sprite list...
    mSprites.push_back(newSprite);                         // ...so add sprite to end of list

    // Maintain total area of sprites to help with atlasing in Finalise method
    mTotalSpriteArea += width * height;
    
    return newSprite.ID;
  }
  catch (const CImgIOException&) // CImg constructor throws an exception - image failed to load
  {
    return INVALID_ID;
  }
}


// Call Finalise when all sprites have been defined on a sprite set. The method prepares an atlas
// of the sprites on a large texture in CPU-memory. Does not upload to the GPU yet, use the Load
// method for that. This method can only be called once and afterward you cannot call DefineSprite
// to add any more sprites (it will do nothing). Returns true on success
bool SpriteSet::Finalise()
{
  if (mAtlasCreated)  return false;  // Can only finalise once

  // Layout all sprites on a large texture, a non-trivial process which doesn't particularly relate
  // to the topic at hand - the code is at the end of the file
  if (!CreateSpriteAtlas()) return false; 

  // Uncomment these two lines if you want to see the atlas texture (for debugging / testing)
  //CImgDisplay display(mAtlasCPU);
  //while (!display.is_closed())  display.wait();

  mAtlasCreated = true;
  return true;
}


//-------------------------------------------------------------------------------------------------
// GPU Residency
//-------------------------------------------------------------------------------------------------

// Bring the sprite set texture into GPU memory and prepare buffers for rendering sprite instances
// Specify the maximum number of sprite instances from this set that will be rendered at once
// Returns true on success
bool SpriteSet::Load(uint32_t maxInstances)
{
  if (mLoaded)
  {
    if (maxInstances != mMaxInstances)  Unload();    // Strange case of user calling Load again with different maxInstances (always check your corners...)
    else                                return true; // Already loaded (note that this is not a failure, so return true)
  }

  uint32_t atlasWidth  = mAtlasCPU.width();
  uint32_t atlasHeight = mAtlasCPU.height();

  // Get pointer to raw data from CPU version of the atlas
  mAtlasCPU.permute_axes("cxyz"); // CImg stores RGB data in seperate planes - convert to usual interleaved format
  D3D11_SUBRESOURCE_DATA atlasData{};
  atlasData.pSysMem     = mAtlasCPU.data(); // Raw RGB data
  atlasData.SysMemPitch = atlasWidth * 4;   // Width of one horizontal line of the CPU texture in bytes

  // Create texture for sprite atlas
  D3D11_TEXTURE2D_DESC textureDesc;
  textureDesc.Width  = atlasWidth;
  textureDesc.Height = atlasHeight;
  textureDesc.MipLevels = 1;
  textureDesc.ArraySize = 1;
  textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.SampleDesc.Count = 1;
  textureDesc.SampleDesc.Quality = 0;
  textureDesc.Usage = D3D11_USAGE_DEFAULT;
  textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  textureDesc.CPUAccessFlags = 0;
  textureDesc.MiscFlags = 0;
  if (FAILED(mDevice->CreateTexture2D(&textureDesc, &atlasData, &mAtlasGPU)))
  {
    mAtlasCPU.permute_axes("yzcx"); // Convert CImg data back to its usual format
    mVertexBuffer.Release();
    return false;
  }
  mAtlasCPU.permute_axes("yzcx");

  // Create shader resource view so sprite atlas can be accessed in shaders
  D3D11_SHADER_RESOURCE_VIEW_DESC textureSRVDesc;
  textureSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  textureSRVDesc.Texture2D.MipLevels = 1;
  textureSRVDesc.Texture2D.MostDetailedMip = 0;
  if (FAILED(mDevice->CreateShaderResourceView(mAtlasGPU, &textureSRVDesc, &mAtlasSRV)))
  {
    mAtlasGPU.Release();
    mVertexBuffer.Release();
    return false;
  }

  //---------------------------------------------------------------------------------------------------------------------

  // Create a CPU-side buffer to collect the sprite instances that are to be rendered this frame
  // Size is from maximum number sprite instances
  mMaxInstances = maxInstances;
  mInstancesToRender = std::make_unique<SpriteRenderData[]>(mMaxInstances);  // Note that this array version of make_unique is from C++14
  mInstancesToRenderEnd = mInstancesToRender.get();

  // Prepare the GPU vertex buffer for rendering sprites from this set. Will be a straight copy of the CPU-buffer
  D3D11_BUFFER_DESC bufferDesc{}; // Initialise all members to 0
	bufferDesc.ByteWidth      = mMaxInstances * sizeof(SpriteRenderData); // Total size of buffer
	bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;      // Will be updated from CPU->GPU regularly
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;   // CPU will write to the buffer
	bufferDesc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	if (FAILED(mDevice->CreateBuffer(&bufferDesc, nullptr, &mVertexBuffer)))  return false;

  mLoaded = true;
  return true;
}


// Release the sprite set texture from GPU memory and the sprite instance buffers
void SpriteSet::Unload()
{
  if (!mLoaded)  return; 

  mVertexBuffer.Release();
  mInstancesToRender.release();
  mAtlasSRV.Release();
  mAtlasGPU.Release();
  mLoaded = false;
}
  

//-------------------------------------------------------------------------------------------------
// Instances / Rendering
//-------------------------------------------------------------------------------------------------

// Add a collection of sprite instances from this set to be rendered this frame. The user calls
// this method once for each collection of instances they want drawn - how the user manages their
// instances is not relevant to this function, only that the instance data is contiguous in the
// 'instances' parameter. When all instance collections have been passed to this method, call the
// Render method (see below) to render them all in one go
// If The *total* number of instances passed to this method between calls to Render exceeds the
// maxInstances value passed to the Load method then the extra instances will not be drawn
// Optionally pass an x,y offset for all instances - useful for scrolling
void SpriteSet::AddToRenderQueue(SpriteInstance* instances, uint32_t number, Vector2f offset /*= {0, 0}*/)
{
  // Guard against adding too many instances
  if (mInstancesToRenderEnd + number > mInstancesToRender.get() + mMaxInstances)  
    number = mMaxInstances - static_cast<uint32_t>((mInstancesToRenderEnd - mInstancesToRender.get())); // Will ignore excess instances
    
  // Merge data from two sources to create the vertex buffer data - the user instance data and the sprite atlas data
  // Note the tight loop and near lack of redundant data - this is very cache friendly
  auto lastInstance = instances + number;
  while (instances != lastInstance)
  {
    mInstancesToRenderEnd->position = instances->position;
    mInstancesToRenderEnd->position.head<2>() += offset;
    mInstancesToRenderEnd->rotation = instances->rotation;
    mInstancesToRenderEnd->scale    = instances->scale;
    mInstancesToRenderEnd->custom1  = instances->custom1;
    auto& sprite = mSprites[instances->ID];
    mInstancesToRenderEnd->size        = sprite.size;
    mInstancesToRenderEnd->anchor      = sprite.anchor;
    mInstancesToRenderEnd->UV          = sprite.UV;
    mInstancesToRenderEnd->atlasIndex  = sprite.atlasIndex;
    mInstancesToRenderEnd++;
    instances++;
  }
}

// Render all the sprite instances that have been passed to the AddInstancesToRender function since
// the last call to this function. Returns true on success
bool SpriteSet::Render()
{
  // The data for the vertex buffer has been collected CPU-side by the AddInstancesToRender function
  // above. Copy that data to the vertex buffer now. Do it all in one go for best performance. 
  // Note that this requires a per-frame CPU->GPU copy, which we know is not ideally efficient after
  // looking at particle systems. However, the sprites are updated CPU-side here so it is necessary
  uint32_t numInstances = static_cast<uint32_t>(mInstancesToRenderEnd - mInstancesToRender.get());
  D3D11_MAPPED_SUBRESOURCE bufferData;
  if (FAILED(mContext->Map(mVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferData)))  return false;
  memcpy(bufferData.pData, mInstancesToRender.get(), numInstances * sizeof(SpriteRenderData));
	mContext->Unmap(mVertexBuffer, 0);

  // Set vertex buffer and texture atlas on the GPU and draw all the instances in one call
	UINT vertexSize = sizeof(SpriteRenderData);
	UINT offset = 0;
  mContext->IASetVertexBuffers(0, 1, &mVertexBuffer.p, &vertexSize, &offset);
 	mContext->PSSetShaderResources(0, 1, &mAtlasSRV.p);
  mContext->Draw(numInstances, 0);

  mInstancesToRenderEnd = mInstancesToRender.get(); // Reset the list of instances to be rendered
  return true;
}


//-------------------------------------------------------------------------------------------------
// Support functions
//-------------------------------------------------------------------------------------------------

// Create a sprite atlas, a large texture that contains all the sprites in this set
// Returns true on success
bool SpriteSet::CreateSpriteAtlas()
{
  if (mSprites.size() == 0)  return false; // Should this be true or false...?  Zero area atlas could cause problems elsewhere so go for false.

  // Sort sprites by width then by height, largest first. Committing larger sprites first will tend to pack the atlas better
  // When finished the sprites need to be put back into ID order (or SpriteIDs will refer to the wrong sprites). This is the
  // reason why the ID is held in the Sprite structure.
  std::sort(mSprites.begin(), mSprites.end(), [](auto& a, auto& b) {
    if (a.size.x() > b.size.x())  return true;
    if (a.size.x() < b.size.x())  return false;
    return a.size.y() > b.size.y();
  });

  // Estimate the atlas area to attempt to fit sprites to. Initially see if we can get a perfect fit
  uint32_t atlasArea = mTotalSpriteArea;
  do
  {
    // Choose atlas width & height - start with a square (sqrt of total area rounded up)
    mAtlasSize.x() = static_cast<uint32_t>(ceil(sqrt(static_cast<float>(atlasArea))));
    mAtlasSize.y() = mAtlasSize.x();

    // If texture dimensions exceed DirectX 11 limits then cannot atlas this sprite set (until I support texture array atlases...)
    if (mAtlasSize.x() > 16384 || mAtlasSize.y() > 16384)
    {
      // Before returning, sort sprites by ID, so SpriteIDs are correct
      std::sort(mSprites.begin(), mSprites.end(), [](auto& a, auto& b) { return a.ID < b.ID; });
      return false;
    }

    // Local structure representing an empty rectangle within the atlas
    struct AtlasRectangle
    {
      Vector2i position;
      Vector2i size;
    };

    // Vector that tracks all the empty rectangles in the atlas as we fill it up with sprites (see algorithm below)
    std::vector<AtlasRectangle> rectangles;
    rectangles.reserve(mSprites.size() * 2 + 1); // Reserve space for maximum number of rectangles generated by the algorithm below

    // At first the whole atlas is the only empty rectangle
    rectangles.push_back(AtlasRectangle{{0, 0}, mAtlasSize});
    auto currentRectangle = rectangles.begin();

    // Set all sprite UVs to an invalid value to indicate they have not yet been added to the atlas.
    // When they are added they will get valid UVs indicating their position, and will be skipped 
    // over in later passes of the algorithm. This means that sprites are repeatedly visited even
    // though they have already been added to the atlas. However, the algorithm works best if the
    // sprites remain sorted and so keeping sprites in place in this way creates a better result.
    // So whilst this approach has an inefficiency, in other approaches it would be expensive to
    // maintain the sorting (linked lists, or shuffling sprites around a lot) 
    for (auto& sprite : mSprites)  sprite.UV.x() = -1;
   

    //---------------------------------------------------------------------------------------------------------------------

    // Altasing algorithm - not perfect, but robust
    //---------------------------------------------
    // Try to fit each sprite into the first rectangle from the empty rectangle list. Each time we
    // find a fit, set the UVs of that sprite and remove it from the search. Now the rectangle will
    // have been divided into two smaller rectangles, one to the right and one below the newly
    // positioned sprite. Either or both may not exist if the sprite exactly matched the dimensions 
    // of the original rectangle.
	  //   +--+--------+
	  //   |XX| Rect 1 |
	  //   +--+--------+
	  //   |   Rect 2  |
	  //   |           |
	  //   +-----------+
	  // Update the current rectangle to be one of these areas, and add the other rectangle (if there
    // is one) to the empty rectangle list. Then continue trying to fit sprites into the current
    // rectangle (which is now smaller). When no further sprites fit the current rectangle, move
    // to the next rectangle. If you can't fit any sprite in any of the remaining rectangles then
    // the sprites won't fit in this size atlas
    uint32_t totalAreaFitted = 0;
    mAtlasSize = {0, 0}; // Reset the atlas size now, and track the actual extent of the used area when adding each sprite
    do
    {
      // Step through sprites until we find one that has not yet been added and fits in current
      // rectangle, or until we run out of sprites
      auto sprite = mSprites.begin();
      while (sprite != mSprites.end() && (sprite->UV.x() >= 0 ||
             sprite->size.x() > currentRectangle->size.x() || sprite->size.y() > currentRectangle->size.y()))  ++sprite;
    
      // If we ran out of sprites, then no sprites fit this rectangle. Step to next rectangle and start searching from the first sprite again
      if (sprite == mSprites.end())
      {
        ++currentRectangle;
        sprite = mSprites.begin();
      }
      else
      {
        // Found a sprite that fits in this rectangle, set its UVs. For now UVs will be pixel
        // positions rather than [0-1] coordinates to help with loading of sprite data below
        sprite->UV.x() = (float)currentRectangle->position.x();
        sprite->UV.y() = (float)currentRectangle->position.y();
        sprite->atlasIndex = 0; // @todo for very large sprite sets that need an array of atlases, not implemented
        
        // Track the maximum used x and y coordinate to determine the final size for the atlas
        if (sprite->UV.x() + sprite->size.x() > mAtlasSize.x())  mAtlasSize.x() = (uint32_t)(sprite->UV.x() + sprite->size.x());
        if (sprite->UV.y() + sprite->size.y() > mAtlasSize.y())  mAtlasSize.y() = (uint32_t)(sprite->UV.y() + sprite->size.y());

        // Update total area fitted to see if we have finished
        totalAreaFitted += (uint32_t)sprite->size.x() * (uint32_t)sprite->size.y();
        if (totalAreaFitted == mTotalSpriteArea)  goto LOAD_SPRITES;  // YES! I used goto!!!

        // Update rectangles (see diagram above)
        if (sprite->size.x() < currentRectangle->size.x())
        {
          if (sprite->size.y() < currentRectangle->size.y())
          {
            // Both sprite dimensions were smaller than rectangle so there will be two rectangles
            // First add new empty rectangle below current rectangle
            Vector2i belowPosition = currentRectangle->position;
            Vector2i belowSize     = currentRectangle->size;
            belowPosition.y() += (uint32_t)sprite->size.y();
            belowSize.y()     -= (uint32_t)sprite->size.y();
            rectangles.push_back(AtlasRectangle{belowPosition, belowSize});
          }
          // Update current rectangle to the empty area to the right
          currentRectangle->position.x() += (uint32_t)sprite->size.x();
          currentRectangle->size.x()     -= (uint32_t)sprite->size.x();
          currentRectangle->size.y()     =  (uint32_t)sprite->size.y();
        }
        else if (sprite->size.y() < currentRectangle->size.y())
        {
          // Sprite fits rectangle horizontally only, so update current rectangle to the empty area below
          currentRectangle->position.y() += (uint32_t)sprite->size.y();
          currentRectangle->size.y()     -= (uint32_t)sprite->size.x();
          currentRectangle->size.x()     =  (uint32_t)sprite->size.x();
        }
        else
        {
          // Sprite fits rectangle perfectly, step to next rectangle and start searching from the first sprite again
          ++currentRectangle;
          sprite = mSprites.begin();
        }
      }
    } while (currentRectangle != rectangles.end());

    // Failed to fit sprites, increase atlas area for next attempt by half of the amount that did not fit (heuristic)
    atlasArea += (mTotalSpriteArea - totalAreaFitted + 1) / 2;

  } while (true); // Try again with bigger atlas...


  //---------------------------------------------------------------------------------------------------------------------

LOAD_SPRITES:
  // Arrive here if we successfully fitted all the sprites
  try
  {
    // Sort sprites by ID, so SpriteIDs are valid again
    std::sort(mSprites.begin(), mSprites.end(), [](auto& a, auto& b) { return a.ID < b.ID; });

    // Create a CPU-side texture for the atlas
    mAtlasCPU.assign(mAtlasSize.x(), mAtlasSize.y(), 1, 4);

    // Load each sprite onto the atlas
    for (auto& sprite : mSprites)
    {
      // Use the pixel position that stored in the sprite UVs, then scale down UVs to correct [0-1] range for shaders
      uint32_t offsetX = (uint32_t)sprite.UV.x();
      uint32_t offsetY = (uint32_t)sprite.UV.y();
      sprite.UV.x() = (sprite.UV.x() + 0.5f) / mAtlasSize.x();  // Add half a pixel to the top/left to stop texture
      sprite.UV.y() = (sprite.UV.y() + 0.5f) / mAtlasSize.y();  // bleeding into neigbouring atlas sprites. See geometry shader

      // Load and copy pixel data
      CImg<uint8_t> spriteImage(sprite.filename.c_str());
      uint32_t spriteWidth  = (uint32_t)sprite.size.x();
      uint32_t spriteHeight = (uint32_t)sprite.size.y();
      bool hasAlpha = (spriteImage.spectrum() == 4);
      for (uint32_t x = 0; x < spriteWidth; ++x)
      {
        for (uint32_t y = 0; y < spriteHeight; ++y)
        {
          mAtlasCPU(offsetX + x, offsetY + y, 0) = spriteImage(x, y, 0);
          mAtlasCPU(offsetX + x, offsetY + y, 1) = spriteImage(x, y, 1);
          mAtlasCPU(offsetX + x, offsetY + y, 2) = spriteImage(x, y, 2);
          mAtlasCPU(offsetX + x, offsetY + y, 3) = (hasAlpha ? spriteImage(x, y, 3) : 255); // Sprite might have alpha or not. The atlas always does
        }
      }
    }
    return true;
  }
  catch (const CImgIOException&) // CImg constructor throws an exception - some kind of image error
  {
    mAtlasCPU.assign(); // Empty the texture
    return false;
  }
}


} // namespaces