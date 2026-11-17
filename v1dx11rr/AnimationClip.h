#pragma once
#include <string>
#include <DirectXMath.h>
#include <unordered_map>
#include <vector>

/*struct PositionKey
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
};*/


struct VectorKey {
    double time;
    DirectX::XMFLOAT3 value;
};

struct QuatKey {
    double time;
    DirectX::XMFLOAT4 value; // Quaternion
};

struct BoneAnimationChannel {
    std::string boneName;
    std::vector<VectorKey> positionKeys;
    std::vector<QuatKey> rotationKeys;
    std::vector<VectorKey> scalingKeys;
};

struct AnimationClip {
    std::string name;
    double duration = 0.0;
    double ticksPerSecond = 25.0;
    std::vector<BoneAnimationChannel> channels;

    const BoneAnimationChannel* FindChannel(const std::string& boneName) const {
        for (const auto& channel : channels) {
            if (channel.boneName == boneName) return &channel;
        }
        return nullptr;
    }
};