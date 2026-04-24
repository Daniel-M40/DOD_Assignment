cbuffer PerFrame : register(b0)
{
    float2 backbufferSize;
    float is3D;
    float padding;
};

struct GSInput
{
    float3 position : position;
    float radius : radius;
    float3 colour : colour;
    float pad : padding;
};

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : UV;
    float3 colour : COLOUR;
    float isNode : ISNODE;
    float depth : DEPTH;
};

[maxvertexcount(4)]
void main(point GSInput input[1], inout TriangleStream<PSInput> output)
{
    PSInput outVertex;

    float depthScale = 1.0f - input[0].position.z * 0.5f;
    float scaledRadius = input[0].radius * depthScale;

    float2 NDCConversion = 2.0f / backbufferSize;
    float2 NDCSize = float2(scaledRadius * 2.0f, scaledRadius * 2.0f) * NDCConversion;

    float2 centre;
    centre.x = input[0].position.x * NDCConversion.x - 1.0f;
    centre.y = -(input[0].position.y * NDCConversion.y - 1.0f);

    outVertex.position.z = input[0].position.z;
    outVertex.position.w = 1.0f;
    outVertex.colour = input[0].colour;
    outVertex.isNode = input[0].pad;
    outVertex.depth = input[0].position.z;

    // Top-left
    outVertex.position.xy = centre + float2(-NDCSize.x, NDCSize.y) * 0.5f;
    outVertex.uv = float2(0.0f, 0.0f);
    output.Append(outVertex);

    // Bottom-left
    outVertex.position.xy = centre + float2(-NDCSize.x, -NDCSize.y) * 0.5f;
    outVertex.uv = float2(0.0f, 1.0f);
    output.Append(outVertex);

    // Top-right
    outVertex.position.xy = centre + float2(NDCSize.x, NDCSize.y) * 0.5f;
    outVertex.uv = float2(1.0f, 0.0f);
    output.Append(outVertex);

    // Bottom-right
    outVertex.position.xy = centre + float2(NDCSize.x, -NDCSize.y) * 0.5f;
    outVertex.uv = float2(1.0f, 1.0f);
    output.Append(outVertex);
}