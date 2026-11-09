#pragma once
#include <string>
#include <DirectXMath.h>
#include <unordered_map>

struct PositionKey
{
    double time;
    DirectX::XMFLOAT3 value;
};

struct RotationKey
{
    double time;
    DirectX::XMFLOAT4 value;
};

struct ScaleKey
{
    double time;
    DirectX::XMFLOAT3 value;
};

struct BoneAnimationChannel
{
    std::string boneName;

    std::vector<PositionKey> positions;
    std::vector<RotationKey> rotations;
    std::vector<ScaleKey> scales;
};

struct AnimationClip
{
    std::string name;
    double duration;
    double ticksPerSecond;

    std::vector<BoneAnimationChannel> channels;
};