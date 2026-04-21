struct VSInput
{
    float3 position : position;
    float radius : radius;
    float3 colour : colour;
    float padding : padding;
};

typedef VSInput GSInput;

GSInput main(VSInput input)
{
    return input;
}