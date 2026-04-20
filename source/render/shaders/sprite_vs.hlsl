// Sprite vertex shader. Only copies the data to the geometry shader, which does the vertex work

// The input to the vertex shader is the same structure as SpriteRenderData
struct VSInput
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

// Input to the geometry shader is output from vertex shader - this shader just copies entire structure
typedef VSInput GSInput;

GSInput main(VSInput input)
{
  return input;
}