#pragma once

#include <DirectXMath.h>

/*
* @brief Define the CPU/GPU vertex format for skinned meshes.
*/
/*struct SkinnedVertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texcoord;

    DirectX::XMFLOAT4 boneWeights;
    DirectX::XMUINT4 boneIndices;
};*/

struct AnimatedVertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 UV;
    float             Weights[4];
    unsigned int      BoneIndices[4];

    AnimatedVertex()
    {
        Position = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        Normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
        UV = DirectX::XMFLOAT2(0.0f, 0.0f);
        for (int i = 0; i < 4; ++i) {
            Weights[i] = 0.0f;
            BoneIndices[i] = 0;
        }
    }
};