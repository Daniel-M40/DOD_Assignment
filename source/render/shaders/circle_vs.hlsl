cbuffer PerFrame : register(b0)
{
    float2 backbufferSize;
    float is3D;
    float padding;
};

struct VSInput
{
    // Per-vertex (quad corner)
    float2 corner : POSITION;
    
    // Per-instance (circle data)
    float3 position : INST_POSITION;
    float radius : INST_RADIUS;
    float3 colour : INST_COLOUR;
    float pad : INST_PADDING;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : UV;
    float3 colour : COLOUR;
    float isNode : ISNODE;
    float depth : DEPTH;
};

PSInput main(VSInput input)
{
    PSInput output;

    float depthScale = 0.3f + (1.0f - input.position.z) * 0.7f;
    float scaledRadius = input.radius * depthScale;

    float2 NDCConversion = 2.0f / backbufferSize;

    float2 centre;
    centre.x = input.position.x * NDCConversion.x - 1.0f;
    centre.y = -(input.position.y * NDCConversion.y - 1.0f);

    float2 NDCSize = float2(scaledRadius, scaledRadius) * NDCConversion;

    output.position.xy = centre + input.corner * NDCSize;
    output.position.z = input.position.z;
    output.position.w = 1.0f;

    output.uv = input.corner * 0.5f + 0.5f;
    output.colour = input.colour;
    output.isNode = input.pad;
    output.depth = input.position.z;

    return output;
}