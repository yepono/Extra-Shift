Texture2D colorMap  : register(t0); // Low terrain: sand
Texture2D colorMap2 : register(t1); // Mid terrain: mud
Texture2D colorMap3 : register(t2); // High terrain: playground

SamplerState colorSampler : register(s0);

cbuffer cbChangerEveryFrame : register(b0)
{
    matrix worldMatrix;
};

cbuffer cbNeverChanges : register(b1)
{
    matrix viewMatrix;
};

cbuffer cbChangeOnResize : register(b2)
{
    matrix projMatrix;
};

struct VS_Input
{
    float4 pos      : POSITION;
    float2 tex0     : TEXCOORD0;
    float2 blendTex : TEXCOORD1;
    float3 normal   : NORMAL0;
    float3 tangente : NORMAL1;
    float3 binormal : NORMAL2;
};

struct PS_Input
{
    float4 pos            : SV_POSITION;
    float2 tex0           : TEXCOORD0;
    float2 blendTex       : TEXCOORD1;
    float3 normal         : NORMAL0;
    float3 tangent        : NORMAL1;
    float3 binorm         : NORMAL2;
    float terrainHeight   : TEXCOORD2;
};

PS_Input VS_Main(VS_Input vertex)
{
    PS_Input output = (PS_Input)0;

    output.pos = mul(vertex.pos, worldMatrix);
    output.pos = mul(output.pos, viewMatrix);
    output.pos = mul(output.pos, projMatrix);

    output.tex0 = vertex.tex0;
    output.blendTex = vertex.blendTex;

    output.normal = normalize(mul(vertex.normal, worldMatrix));
    output.tangent = normalize(mul(vertex.tangente, worldMatrix));
    output.binorm = normalize(mul(vertex.binormal, worldMatrix));

    // The terrain height is already stored in POSITION.y.
    output.terrainHeight = vertex.pos.y;

    return output;
}

float4 PS_Main(PS_Input input) : SV_TARGET
{
    float4 lowTerrain  = colorMap.Sample(colorSampler, input.tex0);
    float4 midTerrain  = colorMap2.Sample(colorSampler, input.tex0);
    float4 highTerrain = colorMap3.Sample(colorSampler, input.tex0);

    /*
        TerrenoRR calculates height as:

            alturaData[x][y] / 9.0

        A byte has a maximum value of 255, so the maximum terrain
        height is approximately 28.33.
    */
    float height = saturate(input.terrainHeight / 28.0f);

    // Low elevation: coast_sand_02_diff_2k
    float lowWeight = saturate(1.0f - height * 3.0f);

    // Middle elevation: mud_cracked_dry_03_diff_2k
    float midWeight =
        saturate(1.0f - abs(height - 0.5f) * 4.0f);

    // High elevation: baseball_playground_diff_2k
    float highWeight =
        saturate((height - 0.65f) * 3.0f);

    float totalWeight =
        lowWeight +
        midWeight +
        highWeight;

    totalWeight = max(totalWeight, 0.0001f);

    lowWeight /= totalWeight;
    midWeight /= totalWeight;
    highWeight /= totalWeight;

    float4 terrainColor =
        lowTerrain * lowWeight +
        midTerrain * midWeight +
        highTerrain * highWeight;

    float3 ambient = float3(0.1f, 0.1f, 0.1f);

    float3 diffuseDirection =
        normalize(float3(0.5f, -1.0f, 0.0f));

    float3 diffuse =
        saturate(
            dot(-diffuseDirection, normalize(input.normal))
        );

    diffuse = saturate(diffuse + ambient);

    return float4(terrainColor.rgb * diffuse, 1.0f);
}