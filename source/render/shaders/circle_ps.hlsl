struct PSInput
{
    float4 position : SV_Position;
    float2 uv : UV;
    float3 colour : COLOUR;
};

float4 main(PSInput input) : SV_Target
{
    float2 offset = input.uv * 2.0f - 1.0f;
    float dist = dot(offset, offset);

    if (dist > 1.0f)
        discard;

    return float4(input.colour, 1.0f);
}