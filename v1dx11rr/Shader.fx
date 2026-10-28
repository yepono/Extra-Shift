Texture2D colorMap : register(t0);
Texture2D colorMap2 : register(t1);
Texture2D colorMap3 : register(t2);
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
	float4 pos : POSITION;
	float2 tex0 : TEXCOORD0;
	float2 blendTex : TEXCOORD1;
	float3 normal : NORMAL0;
	float3 tangente : NORMAL1;
	float3 binormal : NORMAL2;
};

struct PS_Input
{
    float4 pos : SV_POSITION;
    float2 tex0 : TEXCOORD0;
    float2 blendTex : TEXCOORD1;
    float3 normal : NORMAL0;
    float3 tangent : NORMAL1;
    float3 binorm : NORMAL2;

    float terrainHeight : TEXCOORD2;
};

PS_Input VS_Main(VS_Input vertex)
{
    PS_Input vsOut = (PS_Input)0;

    vsOut.pos = mul(vertex.pos, worldMatrix);
    vsOut.pos = mul(vsOut.pos, viewMatrix);
    vsOut.pos = mul(vsOut.pos, projMatrix);

    vsOut.tex0 = vertex.tex0;
    vsOut.blendTex = vertex.blendTex;

    vsOut.normal = normalize(mul(vertex.normal, worldMatrix));
    vsOut.tangent = normalize(mul(vertex.tangente, worldMatrix));
    vsOut.binorm = normalize(mul(vertex.binormal, worldMatrix));

    vsOut.terrainHeight = vertex.pos.y;

    return vsOut;
}

float4 PS_Main(PS_Input pix) : SV_TARGET
{
//	float4 fColor = float4(1,0,0,1);

	float3 ambient = float3(0.1f, 0.1f, 0.1f);

	float4 grass = colorMap.Sample(colorSampler, pix.tex0);
	float4 rock  = colorMap2.Sample(colorSampler, pix.tex0);
	float4 snow  = colorMap3.Sample(colorSampler, pix.tex0);

	// para convertir la altura del terreno a un rango de 0 a 1
	float height = saturate(pix.terrainHeight / 28.0f);

    // terreno bajo recibe pasto
    float grassWeight = saturate(1.0f - height * 3.0f);

    // terreno medio recibe piedra
    float rockWeight = saturate(1.0f - abs(height - 0.5f) * 4.0f);

    // terreno alto
    float snowWeight = saturate((height - 0.65f) * 3.0f);

    float totalWeight = grassWeight + rockWeight + snowWeight;

    totalWeight = max(totalWeight, 0.0001f);

    grassWeight /= totalWeight;
    grassWeight /= rockWeight;
    grassWeight /= snowWeight;
    
    float3 diffuseDirection =
        normalize(float3(0.5f, -1.0f, 0.0f));

    float diffuse =
        saturate(dot(-diffuseDirection, normalize(pix.normal)));

    diffuse = saturate(diffuse + ambient);

    return float4(terrainColor.rgb * diffuse, 1.0f);
}