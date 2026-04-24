struct PSInput
{
    float4 position : SV_Position;
    float2 uv : UV;
    float3 colour : COLOUR;
    float isNode : ISNODE;
    float depth : DEPTH;
};

cbuffer PerFrame : register(b0)
{
    float2 backbufferSize;
    float is3D;
    float padding;
};

float4 main(PSInput input) : SV_Target
{
    float2 offset = input.uv * 2.0f - 1.0f;
    float dist = dot(offset, offset);

    if (dist > 1.0f)
        discard;

    if (input.isNode > 0.5f)
    {
        if (dist < 0.85f)
            discard;

        float depthFade = 1.0f - input.depth * 0.4f;
        return float4(input.colour * depthFade, 0.8f);
    }

    if (is3D > 0.5f)
    {
        float3 normal = float3(offset.x, offset.y, sqrt(1.0f - dist));
        float3 lightDir = normalize(float3(0.3f, -0.5f, 1.0f));
        float diffuse = max(dot(normal, lightDir), 0.0f);
        float ambient = 0.15f;
        float3 lit = input.colour * (ambient + diffuse * 0.85f);
        float depthFade = 1.0f - input.depth * 0.4f;
        lit *= depthFade;
        return float4(lit, 1.0f);
    }

    return float4(input.colour, 1.0f);
}