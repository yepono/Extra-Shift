cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer BoneBuffer : register(b5)
{
    matrix boneTransforms[128];
};

struct VertexInput
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 uv           : TEXCOORD0;
    float4 weights      : BLENDWEIGHT;
    uint4  boneIndices  : BLENDINDICES;
};

struct PixelInput
{
    float4 position     : SV_POSITION;
    float2 uv           : TEXCOORD0;
    float3 normal       : NORMAL;
};

Texture2D diffuseTex : register(t0);
SamplerState Sampler0 : register(s0);

PixelInput VS(VertexInput input)
{
    PixelInput output;

    // Calculate blended skin matrix
    matrix skinMatrix = 
        boneTransforms[input.boneIndices.x] * input.weights.x +
        boneTransforms[input.boneIndices.y] * input.weights.y +
        boneTransforms[input.boneIndices.z] * input.weights.z +
        boneTransforms[input.boneIndices.w] * input.weights.w;

    // Transform position and normal
    float4 localPosition = mul(float4(input.position, 1.0f), skinMatrix);
    float4 localNormal = mul(float4(input.normal, 0.0f), skinMatrix);

    output.position = mul(localPosition, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.normal = mul(localNormal, worldMatrix).xyz;
    output.uv = input.uv;

    return output;
}

//float4 PS(PixelInput input) : SV_Target
//{
    //float4 textureColor = diffuseTex.Sample(Sampler0, input.uv);
    //float3 lightDir = normalize(float3(0.0f, 1.0f, -1.0f));
    //float diffuse = saturate(dot(normalize(input.normal), lightDir));
    
  //  return textureColor * max(diffuse, 0.2f); // Simple ambient + diffuse
//}

float4 PS(PixelInput input) : SV_Target
{
    // Temporary test color (e.g., solid orange or grey) with basic lighting
    float3 lightDir = normalize(float3(0.0f, 1.0f, -1.0f));
    float diffuse = saturate(dot(normalize(input.normal), lightDir));
    float3 baseColor = float3(0.8f, 0.8f, 0.8f); // Light grey model
    
    return float4(baseColor * max(diffuse, 0.3f), 1.0f);
}