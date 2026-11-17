#pragma once
#include <string>
#include <DirectXMath.h>
#include <unordered_map>
#include <vector>

/*struct BoneInfo
{
    std::string name;
    int parentIndex;

    DirectX::XMFLOAT4X4 offsetMatrix;
    DirectX::XMFLOAT4X4 localTransform;
    DirectX::XMFLOAT4X4 globalTransform;
    DirectX::XMFLOAT4X4 finalTransform;
    std::unordered_map<std::string, unsigned int> boneMapping;
};*/

struct SkeletonNode
{
    std::string name;
    int parentIndex = -1;
    DirectX::XMFLOAT4X4 localTransform;
    std::vector<int> children;
};

struct BoneInfo
{
    std::string name;
    DirectX::XMFLOAT4X4 offsetMatrix; // Inverse bind matrix
};

struct Skeleton
{
    std::vector<SkeletonNode> nodes;
    std::vector<BoneInfo> bones;
    std::vector<DirectX::XMFLOAT4X4> finalMatrices; // Uploaded to GPU
};