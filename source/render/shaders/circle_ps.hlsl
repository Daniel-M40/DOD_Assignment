struct PSInput
{
    float4 position : SV_Position;
    float2 uv : UV;
    float3 colour : COLOUR;
    float isNode : ISNODE;
};

float4 main(PSInput input) : SV_Target
{
    float2 offset = input.uv * 2.0f - 1.0f;
    float dist = dot(offset, offset);

    if (dist > 1.0f)
        discard;

    if (input.isNode > 0.5f)
    {
        if (dist < 0.92f)
            discard;

        return float4(input.colour, 0.5f);
    }

    return float4(input.colour, 1.0f);
}