// Sprite pixel shader. Very straightforward lookup from texture. The geometry shader did the work.

struct VSOutput
{
	float4 position : SV_Position;
  float2 UV       : UV;
};

Texture2D    SpriteAtlas        : register(t0); // Sprite atlas contains all the sprites for a sprite set
SamplerState BilinearClampNoMip : register(s0); // Use bilinear for sprites, mip-mapping (trilinear) adds complexities to sprite atlasing

float4 main(VSOutput input) : SV_Target
{
  // Pixel shader just needs to sample the sprite atlas, the UVs were calculated in earlier shaders
	float4 colour = SpriteAtlas.Sample(BilinearClampNoMip, input.UV); 

  clip(colour.a - 0.1f); // Discard pixels with alpha < 0.1, alpha testing for cutout sprites

  return colour;
}

