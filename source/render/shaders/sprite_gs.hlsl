// Sprite geometry shader. Expands sprite point to quad taking account of scale, rotation, and
// sprite anchor. Sprite texture is stored on a sprite atlas, this shader prepares the correct UVs

cbuffer PerSpriteSet : register(b0)
{
  float2 backbufferSize;
  float2 atlasSize;
};

// Copied directly from the vertex shader - the input is the same structure as SpriteRenderData 
struct GSInput
{
  // Note that HLSL requires "semantics", the word after the colon. These are actually ignored
  // unless they begin with SV_ (system value), so I just copy the variable name these days

	float3 position : position;   // XY is position on screen in pixels, with 0,0 at the top left. Z is depth order, 0->1
  float  rotation : rotation;   // In radians, anticlockwise
  float2 scale    : scale;      // XY scaling
  uint   custom1  : custom1;    // ???
  uint   custom2  : custom2;    // ???
  float2 size     : size;       // Size of this sprite in pixels
  float2 anchor   : anchor;     // Anchor position in pixels, with 0,0 being top left. This point is used for positioning/rotation
  float2 UV       : UV;         // UV for top left of sprite in the sprite atlas
  uint   texIndex : atlasIndex; // Index into sprite atlas array (only needed for very large sprite sets, so usually 0)
  uint   custom3  : custom3;    // ???
};

// Pixel shader will only need position and UV
struct PSInput
{
  float4 position : SV_Position;
  float2 UV       : UV;
};

// Create a quad for the sprite based on the shader input (which came from the SpriteRenderData structures held by the sprite set in the C++ code)
[maxvertexcount(4)]  
void main(point GSInput input[1],	inout TriangleStream<PSInput> output)
{
	PSInput outVertex;

  // @todo rotation

  // The sprite instance data uses viewport coordinates measured in pixels. We can't use viewport coordinates in  
  // shaders before the pixel shader because positions will be scaled to the screen in the rasterization stage.
  // Instead we use "normalised device coordinates" (NDC). This is the usual way of working in 2D in shaders before 
  // the pixel shader. In NDC x & y range from (-1,-1) to (1,1) from bottom-left to top-right of the viewport. 
  // z is 0 to 1 for depth, and w must be 1

  // Convert sprite size to UV coordinates range (0->1) to get area on the sprite atlas to use, do this before
  // adjusting sprite size by user scaling (user scaling doesn't affect the atlas)
  float2 UVSize = (input[0].size - 0.5f) / atlasSize; // Subtract half a pixel from the right/bottom to stop texture
                                                      // bleeding into neigbouring atlas sprites. See CreateSpriteAtlas

  // Apply user scaling to the sprite size and anchor
  input[0].size   *= input[0].scale;
  input[0].anchor *= input[0].scale;

  // Convert sprite position & size from viewport coordinates to NDC. Position is scaled to range (0->2) then 1 is
  // subtracted to shift the range to (-1->1). Size just needs to be scaled down (size isn't affected by shifting range)
  float2 NDCConversion = 2.0f / backbufferSize;
  float2 NDCSize  = input[0].size * NDCConversion;
	outVertex.position.xy = (input[0].position.xy - input[0].anchor) * NDCConversion - float2(1,1); 
  outVertex.position.y = -outVertex.position.y; // NDC y axis is up, but user's coordinates y axis goes down.
  
  // Z is sprite depth and just copied, w must be 1 for NDC coordinates
  outVertex.position.z  = input[0].position.z;
  outVertex.position.w  = 1.0f;
  
  // Copy top-left UV and create first corner vertex
  outVertex.UV = input[0].UV;
	output.Append(outVertex); // Top-left corner of quad

  // Reuse the vertex from the last corner - use sizes in NDC and UV space to offset to the other corners
  outVertex.position.y -= NDCSize.y;
  outVertex.UV.y       += UVSize.y;
	output.Append(outVertex); // Top-left corner

  outVertex.position.x += NDCSize.x;
  outVertex.position.y += NDCSize.y;
  outVertex.UV.x += UVSize.x;
  outVertex.UV.y -= UVSize.y;
	output.Append(outVertex); // Bottom-right corner

  outVertex.position.y -= NDCSize.y;
  outVertex.UV.y       += UVSize.y;
	output.Append(outVertex); // Top-right corner
}
