[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{

struct VS_Input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD0;

    float4 boneWeights : BLENDWEIGHT;
    uint4 boneIndices : BLENDINDICES;
};

Texture2D colorMap : register(t0);
Texture2D specMap : register(t1);
SamplerState colorSampler : register(s0);

cbuffer cbWorld : register(b0)
{
    matrix worldMatrix;
};

cbuffer cbView : register(b1)
{
    matrix viewMatrix;
};

cbuffer cbProjection : register(b2)
{
    matrix projMatrix;
};

cbuffer cbCamera : register(b3)
{
    float3 cameraPos;
    float padding;
};

cbuffer cbMaterial : register(b4)
{
    float specForce;
    float3 materialPadding;
};

cbuffer cbBones : register(b5)
{
    matrix boneMatrices[128];
};
}