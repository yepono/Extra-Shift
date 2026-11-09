#pragma once

#include <DirectXMath.h>

/**
* @brief Define the CPU/GPU vertex format for skinned meshes.
*/
struct SkinnedVertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texcoord;

    DirectX::XMFLOAT4 boneWeights;
    DirectX::XMUINT4 boneIndices;
};