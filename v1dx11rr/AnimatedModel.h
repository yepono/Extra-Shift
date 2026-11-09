#pragma once
#include <string>
class AnimatedModel
{
private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    ID3D11VertexShader* vertexShader;
    ID3D11PixelShader* pixelShader;
    ID3D11InputLayout* inputLayout;

    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;
    ID3D11Buffer* boneBuffer;

    ID3D11Buffer* worldBuffer;
    ID3D11Buffer* viewBuffer;
    ID3D11Buffer* projectionBuffer;

    std::vector<MeshPart> meshParts;
    Skeleton skeleton;
    std::vector<AnimationClip> animations;

    int currentAnimation;
    float animationTime;
    bool looping;

public:
    bool Load(const char* filename);
    void Update(float deltaTime);
    void Draw(
        const D3DXMATRIX& view,
        const D3DXMATRIX& projection,
        const D3DXMATRIX& world);

    void PlayAnimation(const std::string& name);
};
