#pragma once
#include <string>
#include <DirectXMath.h>
#include <unordered_map>

struct BoneInfo
{
    std::string name;
    int parentIndex;

    DirectX::XMFLOAT4X4 offsetMatrix;
    DirectX::XMFLOAT4X4 localTransform;
    DirectX::XMFLOAT4X4 globalTransform;
    DirectX::XMFLOAT4X4 finalTransform;
    std::unordered_map<std::string, unsigned int> boneMapping;
};