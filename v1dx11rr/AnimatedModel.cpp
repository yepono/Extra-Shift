#include "AnimatedModel.h"

#include <d3dcompiler.h>
#include <d3dx11.h>

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "FBXImporter.h"

#include <DirectXMath.h>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

struct BoneConstants
{
    DirectX::XMFLOAT4X4 bones[128];
};


namespace
{
    struct SceneConstants
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    template <typename T>
    void ReleaseCom(T*& object)
    {
        if (object)
        {
            object->Release();
            object = nullptr;
        }
    }

    DirectX::XMVECTOR SampleVectorKeys(
        const std::vector<VectorKey>& keys,
        double time,
        DirectX::FXMVECTOR fallback)
    {
        if (keys.empty())
            return fallback;

        if (time <= keys.front().time)
        {
            const DirectX::XMFLOAT3& value = keys.front().value;
            return DirectX::XMVectorSet(value.x, value.y, value.z, 0.0f);
        }

        if (time >= keys.back().time)
        {
            const DirectX::XMFLOAT3& value = keys.back().value;
            return DirectX::XMVectorSet(value.x, value.y, value.z, 0.0f);
        }

        for (size_t i = 0; i + 1 < keys.size(); ++i)
        {
            const VectorKey& first = keys[i];
            const VectorKey& second = keys[i + 1];

            if (time <= second.time)
            {
                const double span = second.time - first.time;
                const float amount = span > 0.0
                    ? static_cast<float>((time - first.time) / span)
                    : 0.0f;

                const DirectX::XMVECTOR firstValue =
                    DirectX::XMVectorSet(
                        first.value.x,
                        first.value.y,
                        first.value.z,
                        0.0f);

                const DirectX::XMVECTOR secondValue =
                    DirectX::XMVectorSet(
                        second.value.x,
                        second.value.y,
                        second.value.z,
                        0.0f);

                return DirectX::XMVectorLerp(firstValue, secondValue, amount);
            }
        }

        const DirectX::XMFLOAT3& value = keys.back().value;
        return DirectX::XMVectorSet(value.x, value.y, value.z, 0.0f);
    }

    DirectX::XMVECTOR SampleRotationKeys(
        const std::vector<QuatKey>& keys,
        double time,
        DirectX::FXMVECTOR fallback)
    {
        if (keys.empty())
            return fallback;

        if (time <= keys.front().time)
        {
            const DirectX::XMFLOAT4& value = keys.front().value;
            return DirectX::XMQuaternionNormalize(
                DirectX::XMVectorSet(value.x, value.y, value.z, value.w));
        }

        if (time >= keys.back().time)
        {
            const DirectX::XMFLOAT4& value = keys.back().value;
            return DirectX::XMQuaternionNormalize(
                DirectX::XMVectorSet(value.x, value.y, value.z, value.w));
        }

        for (size_t i = 0; i + 1 < keys.size(); ++i)
        {
            const QuatKey& first = keys[i];
            const QuatKey& second = keys[i + 1];

            if (time <= second.time)
            {
                const double span = second.time - first.time;
                const float amount = span > 0.0
                    ? static_cast<float>((time - first.time) / span)
                    : 0.0f;

                const DirectX::XMVECTOR firstValue =
                    DirectX::XMVectorSet(
                        first.value.x,
                        first.value.y,
                        first.value.z,
                        first.value.w);

                const DirectX::XMVECTOR secondValue =
                    DirectX::XMVectorSet(
                        second.value.x,
                        second.value.y,
                        second.value.z,
                        second.value.w);

                return DirectX::XMQuaternionNormalize(
                    DirectX::XMQuaternionSlerp(firstValue, secondValue, amount));
            }
        }

        const DirectX::XMFLOAT4& value = keys.back().value;
        return DirectX::XMQuaternionNormalize(
            DirectX::XMVectorSet(value.x, value.y, value.z, value.w));
    }

    bool CreateWhiteTexture(
        ID3D11Device* device,
        ID3D11ShaderResourceView** shaderResourceView)
    {
        if (!device || !shaderResourceView)
            return false;

        const unsigned int whitePixel = 0xffffffff;

        D3D11_TEXTURE2D_DESC textureDescription = {};
        textureDescription.Width = 1;
        textureDescription.Height = 1;
        textureDescription.MipLevels = 1;
        textureDescription.ArraySize = 1;
        textureDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDescription.SampleDesc.Count = 1;
        textureDescription.Usage = D3D11_USAGE_IMMUTABLE;
        textureDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initialData = {};
        initialData.pSysMem = &whitePixel;
        initialData.SysMemPitch = sizeof(whitePixel);

        ID3D11Texture2D* texture = nullptr;
        HRESULT result = device->CreateTexture2D(
            &textureDescription,
            &initialData,
            &texture);

        if (FAILED(result))
            return false;

        result = device->CreateShaderResourceView(
            texture,
            nullptr,
            shaderResourceView);

        texture->Release();
        return SUCCEEDED(result);
    }

    const char* const vertexShaderSource = R"(
cbuffer SceneConstants : register(b0)
{
    row_major float4x4 worldMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

cbuffer BoneConstants : register(b1)
{
    row_major float4x4 bones[128];
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 weights : BLENDWEIGHT;
    uint4 boneIndices : BLENDINDICES;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 viewPos : TEXCOORD2;      // espacio de camara (especular)
    float3 viewNormal : TEXCOORD3;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 localPosition = float4(input.position, 1.0f);
    float3 localNormal = input.normal;

    float weightSum =
        input.weights.x +
        input.weights.y +
        input.weights.z +
        input.weights.w;

    if (weightSum > 0.0001f)
    {
        float4 skinnedPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
        float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);

        [unroll]
        for (uint i = 0; i < 4; ++i)
        {
            float weight = input.weights[i];

            if (weight > 0.0f)
            {
                uint boneIndex = min(input.boneIndices[i], 127u);
                skinnedPosition +=
                    mul(localPosition, bones[boneIndex]) * weight;

                skinnedNormal +=
                    mul(float4(localNormal, 0.0f), bones[boneIndex]).xyz * weight;
            }
        }

        localPosition = skinnedPosition;
        localNormal = normalize(skinnedNormal);
    }

    float4 worldPosition = mul(localPosition, worldMatrix);
    output.position = mul(mul(worldPosition, viewMatrix), projectionMatrix);
    output.normal = normalize(mul(float4(localNormal, 0.0f), worldMatrix).xyz);
    output.viewPos = mul(worldPosition, viewMatrix).xyz;
    output.viewNormal = mul(float4(output.normal, 0.0f), viewMatrix).xyz;
    output.uv = input.uv;

    return output;
}
)";

    const char* const pixelShaderSource = R"(
cbuffer SceneConstants : register(b0)
{
    row_major float4x4 worldMatrix;
    row_major float4x4 viewMatrix;
    row_major float4x4 projectionMatrix;
};

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : TEXCOORD1;
    float3 viewPos : TEXCOORD2;
    float3 viewNormal : TEXCOORD3;
};

// Iluminacion ambiental + difusa + especular (Blinn-Phong), con la misma
// luz direccional que Modelo.fx. La especular se calcula en espacio de
// camara: ahi el ojo esta en el origen.
float4 PSMain(PSInput input) : SV_TARGET
{
    float4 albedo = diffuseTexture.Sample(diffuseSampler, input.uv);

    float3 luzMundo = normalize(-float3(0.5f, -1.0f, 0.0f));
    float3 normal = normalize(input.normal);

    float ambiente = 0.25f;
    float difusa = saturate(dot(normal, luzMundo)) * 0.75f;

    float3 luzVista = normalize(mul(float4(luzMundo, 0.0f), viewMatrix).xyz);
    float3 normalVista = normalize(input.viewNormal);
    float3 alOjo = normalize(-input.viewPos);
    float3 medio = normalize(luzVista + alOjo);
    float especular = pow(saturate(dot(normalVista, medio)), 32.0f) * 0.35f;
    especular *= step(0.0f, dot(normal, luzMundo));

    float3 color = albedo.rgb * (ambiente + difusa) + especular;
    return float4(saturate(color), albedo.a);
}
)";
}

/**
Initialize an animated model
*/
AnimatedModel::AnimatedModel(
    ID3D11Device* device,
    ID3D11DeviceContext* context,
    float positionX,
    float positionZ)
    : m_device(device),
    m_context(context),
    m_importer(new FBXImporter()),
    m_posX(positionX),
    m_posZ(positionZ)
{
    DirectX::XMFLOAT4X4 identity;
    DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
    std::memcpy(m_globalInverseRoot, &identity, sizeof(identity));
}

AnimatedModel::~AnimatedModel()
{
    for (MeshBuffers& mesh : m_meshBuffers)
    {
        ReleaseCom(mesh.vertexBuffer);
        ReleaseCom(mesh.indexBuffer);
        ReleaseCom(mesh.diffuseTexture);
    }

    ReleaseCom(m_vertexShader);
    ReleaseCom(m_pixelShader);
    ReleaseCom(m_inputLayout);
    ReleaseCom(m_boneConstantBuffer);
    ReleaseCom(m_matrixConstantBuffer);
    ReleaseCom(m_sampler);
}

void AnimatedModel::CreateShadersAndInputLayout()
{
    if (!m_device)
        return;

    ID3DBlob* vertexShaderBlob = nullptr;
    ID3DBlob* pixelShaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT result = D3DCompile(
        vertexShaderSource,
        strlen(vertexShaderSource),
        "AnimatedModelVS",
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &vertexShaderBlob,
        &errorBlob);

    if (FAILED(result))
    {
        if (errorBlob)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));

        ReleaseCom(errorBlob);
        ReleaseCom(vertexShaderBlob);
        return;
    }

    ReleaseCom(errorBlob);

    result = D3DCompile(
        pixelShaderSource,
        strlen(pixelShaderSource),
        "AnimatedModelPS",
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS,
        0,
        &pixelShaderBlob,
        &errorBlob);

    if (FAILED(result))
    {
        if (errorBlob)
            OutputDebugStringA(static_cast<const char*>(errorBlob->GetBufferPointer()));

        ReleaseCom(errorBlob);
        ReleaseCom(vertexShaderBlob);
        ReleaseCom(pixelShaderBlob);
        return;
    }

    ReleaseCom(errorBlob);

    result = m_device->CreateVertexShader(
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        nullptr,
        &m_vertexShader);

    if (FAILED(result))
    {
        ReleaseCom(vertexShaderBlob);
        ReleaseCom(pixelShaderBlob);
        return;
    }

    result = m_device->CreatePixelShader(
        pixelShaderBlob->GetBufferPointer(),
        pixelShaderBlob->GetBufferSize(),
        nullptr,
        &m_pixelShader);

    if (FAILED(result))
    {
        ReleaseCom(vertexShaderBlob);
        ReleaseCom(pixelShaderBlob);
        return;
    }

    const D3D11_INPUT_ELEMENT_DESC inputElements[] =
    {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(AnimatedVertex, Position)),     D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(AnimatedVertex, Normal)),       D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, static_cast<UINT>(offsetof(AnimatedVertex, UV)),           D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(AnimatedVertex, Weights)),      D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, static_cast<UINT>(offsetof(AnimatedVertex, BoneIndices)),  D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    result = m_device->CreateInputLayout(
        inputElements,
        ARRAYSIZE(inputElements),
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        &m_inputLayout);

    ReleaseCom(vertexShaderBlob);
    ReleaseCom(pixelShaderBlob);

    if (FAILED(result))
        return;

    D3D11_BUFFER_DESC bufferDescription = {};
    bufferDescription.Usage = D3D11_USAGE_DEFAULT;
    bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    bufferDescription.ByteWidth = sizeof(SceneConstants);
    result = m_device->CreateBuffer(
        &bufferDescription,
        nullptr,
        &m_matrixConstantBuffer);

    if (FAILED(result))
        return;

    bufferDescription.ByteWidth = sizeof(BoneConstants);
    result = m_device->CreateBuffer(
        &bufferDescription,
        nullptr,
        &m_boneConstantBuffer);

    if (FAILED(result))
        return;

    D3D11_SAMPLER_DESC samplerDescription = {};
    samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDescription.MaxAnisotropy = 1;
    samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDescription.MinLOD = 0.0f;
    samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;

    m_device->CreateSamplerState(&samplerDescription, &m_sampler);
}

bool AnimatedModel::Load(
    const std::string& filepath,
    const std::string& diffuseTextureOverride)
{
    if (!m_device || !m_context || filepath.empty())
        return false;

    if (!m_meshBuffers.empty() || !m_importer -> skeleton.nodes.empty())
        return false;

    if (!m_importer->Load(filepath))
        return false;

    if (m_importer->subMeshes.empty())
        return false;

    // Limites locales de la malla (pose de bind) para las coliciones.
    m_tieneLimites = false;
    {
        float minimo[3] = { 1.0e30f, 1.0e30f, 1.0e30f };
        float maximo[3] = { -1.0e30f, -1.0e30f, -1.0e30f };

        for (size_t i = 0; i < m_importer->subMeshes.size(); ++i)
        {
            const SubMesh& subMesh = m_importer->subMeshes[i];

            for (size_t j = 0; j < subMesh.vertices.size(); ++j)
            {
                const DirectX::XMFLOAT3& p = subMesh.vertices[j].Position;

                if (p.x < minimo[0]) minimo[0] = p.x;
                if (p.y < minimo[1]) minimo[1] = p.y;
                if (p.z < minimo[2]) minimo[2] = p.z;

                if (p.x > maximo[0]) maximo[0] = p.x;
                if (p.y > maximo[1]) maximo[1] = p.y;
                if (p.z > maximo[2]) maximo[2] = p.z;
            }
        }

        if (minimo[0] <= maximo[0])
        {
            for (int i = 0; i < 3; ++i)
            {
                m_minLocal[i] = minimo[i];
                m_maxLocal[i] = maximo[i];
            }

            m_tieneLimites = true;
        }
    }

    if (m_importer->skeleton.bones.size() > 128)
    {
        OutputDebugStringA("AnimatedModel: model has more than 128 bones.\n");
        return false;
    }

    CreateShadersAndInputLayout();

    if (!m_vertexShader ||
        !m_pixelShader ||
        !m_inputLayout ||
        !m_matrixConstantBuffer ||
        !m_boneConstantBuffer ||
        !m_sampler)
    {
        return false;
    }

    const DirectX::XMMATRIX rootMatrix =
        DirectX::XMLoadFloat4x4(&m_importer->skeleton.nodes[0].localTransform);

    DirectX::XMVECTOR determinant;
    const DirectX::XMMATRIX inverseRoot =
        DirectX::XMMatrixInverse(&determinant, rootMatrix);


    DirectX::XMFLOAT4X4 inverseRootFloat;

    if (std::fabs(DirectX::XMVectorGetX(determinant)) > 1.0e-8f)
    {
        DirectX::XMStoreFloat4x4(&inverseRootFloat, inverseRoot);
    }
    else
    {
        DirectX::XMStoreFloat4x4(
            &inverseRootFloat,
            DirectX::XMMatrixIdentity());
    }

    std::memcpy(
        m_globalInverseRoot,
        &inverseRootFloat,
        sizeof(inverseRootFloat));

    m_meshBuffers.resize(m_importer->subMeshes.size());

    for (size_t i = 0; i < m_importer->subMeshes.size(); ++i)
    {
        const SubMesh& source = m_importer->subMeshes[i];
        MeshBuffers& destination = m_meshBuffers[i];

        if (source.vertices.empty() || source.indices.empty())
            continue;

        if (source.vertices.size() >
                (std::numeric_limits<UINT>::max)() / sizeof(AnimatedVertex) ||
            source.indices.size() >
                (std::numeric_limits<UINT>::max)() / sizeof(unsigned int))
        {
            return false;
        }

        D3D11_BUFFER_DESC vertexDescription = {};
        vertexDescription.ByteWidth =
            static_cast<UINT>(source.vertices.size() * sizeof(AnimatedVertex));
        vertexDescription.Usage = D3D11_USAGE_IMMUTABLE;
        vertexDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexData = {};
        vertexData.pSysMem = source.vertices.data();

        HRESULT result = m_device->CreateBuffer(
            &vertexDescription,
            &vertexData,
            &destination.vertexBuffer);

        if (FAILED(result))
            return false;

        D3D11_BUFFER_DESC indexDescription = {};
        indexDescription.ByteWidth =
            static_cast<UINT>(source.indices.size() * sizeof(unsigned int));
        indexDescription.Usage = D3D11_USAGE_IMMUTABLE;
        indexDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = source.indices.data();

        result = m_device->CreateBuffer(
            &indexDescription,
            &indexData,
            &destination.indexBuffer);

        if (FAILED(result))
            return false;

        destination.indexCount = static_cast<UINT>(source.indices.size());

        const std::string& diffusePath =
            diffuseTextureOverride.empty()
            ? source.diffuseTexturePath
            : diffuseTextureOverride;

        bool textureLoaded = false;

        if (!diffusePath.empty() && diffusePath[0] != '*')
        {
            std::string texturePath = diffusePath;

            const bool isAbsolute =
                texturePath[0] == '/' ||
                texturePath[0] == '\\' ||
                (texturePath.size() > 1 && texturePath[1] == ':');

            // Material paths are relative to the FBX. An explicit override
            // is relative to the application's working directory.
            if (diffuseTextureOverride.empty() && !isAbsolute)
                texturePath = m_importer->directory + texturePath;

            result = D3DX11CreateShaderResourceViewFromFileA(
                m_device,
                texturePath.c_str(),
                nullptr,
                nullptr,
                &destination.diffuseTexture,
                nullptr);

            textureLoaded = SUCCEEDED(result);

            if (!textureLoaded)
            {
                OutputDebugStringA(
                    ("AnimatedModel: failed to load texture: " +
                     texturePath + "\n").c_str());
            }
        }

        if (!textureLoaded &&
            !CreateWhiteTexture(m_device, &destination.diffuseTexture))
        {
            return false;
        }
    }

    Update(0.0f);
    return true;
}

bool AnimatedModel::LoadAnimationFile(
    const std::string& filepath,
    const std::string& clipName)
{
    if (!m_importer ||
        m_importer->skeleton.nodes.empty())
    {
        OutputDebugStringA(
            "Load the character model before its animation files.\n");
        return false;
    }

    return m_importer->LoadAnimationFile(filepath, clipName);
}

void AnimatedModel::PlayAnimation(const std::string& name, bool loop)
{
    if (name == m_currentAnimation && loop == m_loopAnimation)
        return;

    const auto animation = m_importer->animations.find(name);

    if (animation == m_importer->animations.end())
    {
        OutputDebugStringA(
            "AnimatedModel: requested animation was not found.\n");
        return;
    }

    m_currentAnimation = name;
    m_loopAnimation = loop;
    m_animationFinished = false;
    m_animationTime = 0.0f;
    Update(0.0f);
}

void AnimatedModel::Update(float deltaTime)
{
    if (m_importer->skeleton.nodes.empty())
        return;

    const AnimationClip* clip = nullptr;
    const auto animation = m_importer->animations.find(m_currentAnimation);

    if (animation != m_importer->animations.end())
    {
        clip = &animation->second;

        if (std::isfinite(deltaTime) && deltaTime > 0.0f)
        {
            m_animationTime +=
                deltaTime * static_cast<float>(clip->ticksPerSecond);
        }

        if (clip->duration > 0.0)
        {
            if (m_loopAnimation)
            {
                m_animationTime = static_cast<float>(
                    std::fmod(m_animationTime, clip->duration));
            }
            else if (m_animationTime >= clip->duration)
            {
                m_animationTime = static_cast<float>(clip->duration);
                m_animationFinished = true;
            }
        }
        else if (!m_loopAnimation)
        {
            m_animationFinished = true;
        }
    }

    for (DirectX::XMFLOAT4X4& matrix :
        m_importer->skeleton.finalMatrices)
    {
        DirectX::XMStoreFloat4x4(
            &matrix,
            DirectX::XMMatrixIdentity());
    }

    DirectX::XMFLOAT4X4 identity;
    DirectX::XMStoreFloat4x4(
        &identity,
        DirectX::XMMatrixIdentity());

    TraverseHierarchy(0, &identity._11, clip);
}

void AnimatedModel::TraverseHierarchy(
    int nodeIndex,
    const float* parentMatrix,
    const AnimationClip* clip)
{
    if (!parentMatrix ||
        nodeIndex < 0 ||
        static_cast<size_t>(nodeIndex) >= m_importer->skeleton.nodes.size())
    {
        return;
    }

    DirectX::XMFLOAT4X4 parentFloat;
    std::memcpy(&parentFloat, parentMatrix, sizeof(parentFloat));

    const DirectX::XMMATRIX parent =
        DirectX::XMLoadFloat4x4(&parentFloat);
    if (nodeIndex < 0 ||
        static_cast<size_t>(nodeIndex) >= m_importer->skeleton.nodes.size())
    {
        return;
    }

    const SkeletonNode& node =
        m_importer->skeleton.nodes[static_cast<size_t>(nodeIndex)];

    DirectX::XMVECTOR scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR rotation = DirectX::XMQuaternionIdentity();
    DirectX::XMVECTOR translation = DirectX::XMVectorZero();

    const DirectX::XMMATRIX bindLocal =
        DirectX::XMLoadFloat4x4(&node.localTransform);

    DirectX::XMVECTOR bindScale;
    DirectX::XMVECTOR bindRotation;
    DirectX::XMVECTOR bindTranslation;

    if (DirectX::XMMatrixDecompose(
            &bindScale,
            &bindRotation,
            &bindTranslation,
            bindLocal))
    {
        scale = bindScale;
        rotation = bindRotation;
        translation = bindTranslation;
    }

    if (clip)
    {
        const BoneAnimationChannel* channel =
            clip->FindChannel(node.name);

        if (channel)
        {
            scale = SampleVectorKeys(
                channel->scalingKeys,
                m_animationTime,
                scale);

            rotation = SampleRotationKeys(
                channel->rotationKeys,
                m_animationTime,
                rotation);

            translation = SampleVectorKeys(
                channel->positionKeys,
                m_animationTime,
                translation);
        }
    }

    const DirectX::XMMATRIX localMatrix =
        DirectX::XMMatrixScalingFromVector(scale) *
        DirectX::XMMatrixRotationQuaternion(rotation) *
        DirectX::XMMatrixTranslationFromVector(translation);

    const DirectX::XMMATRIX globalMatrix =
        localMatrix * parent;

    DirectX::XMFLOAT4X4 inverseRootFloat;
    std::memcpy(
        &inverseRootFloat,
        m_globalInverseRoot,
        sizeof(inverseRootFloat));

    const DirectX::XMMATRIX globalInverseRoot =
        DirectX::XMLoadFloat4x4(&inverseRootFloat);

    for (size_t boneIndex = 0;
         boneIndex < m_importer->skeleton.bones.size();
         ++boneIndex)
    {
        const BoneInfo& bone = m_importer->skeleton.bones[boneIndex];

        if (bone.name == node.name)
        {
            const DirectX::XMMATRIX offsetMatrix =
                DirectX::XMLoadFloat4x4(&bone.offsetMatrix);

            const DirectX::XMMATRIX finalMatrix =
                offsetMatrix * globalMatrix * globalInverseRoot;

            DirectX::XMStoreFloat4x4(
                &m_importer->skeleton.finalMatrices[boneIndex],
                finalMatrix);
        }
    }

    DirectX::XMFLOAT4X4 globalFloat;
    DirectX::XMStoreFloat4x4(&globalFloat, globalMatrix);

    for (int childIndex : node.children)
    {
        TraverseHierarchy(
            childIndex,
            &globalFloat._11,
            clip);
    }

}

void AnimatedModel::Draw(
    const float* view,
    const float* projection,
    const float* world)
{
    if (!view ||
        !projection ||
        !world ||
        !m_context ||
        !m_importer ||
        !m_vertexShader ||
        !m_pixelShader ||
        !m_inputLayout ||
        !m_matrixConstantBuffer ||
        !m_boneConstantBuffer ||
        !m_sampler)
    {
        return;
    }

    SceneConstants sceneConstants = {};
    std::memcpy(&sceneConstants.world, world, sizeof(sceneConstants.world));
    std::memcpy(&sceneConstants.view, view, sizeof(sceneConstants.view));
    std::memcpy(
        &sceneConstants.projection,
        projection,
        sizeof(sceneConstants.projection));

    m_context->UpdateSubresource(
        m_matrixConstantBuffer,
        0,
        nullptr,
        &sceneConstants,
        0,
        0);

    BoneConstants boneConstants = {};
    for (size_t i = 0; i < ARRAYSIZE(boneConstants.bones); ++i)
    {
        if (i < m_importer->skeleton.finalMatrices.size())
        {
            boneConstants.bones[i] =
                m_importer->skeleton.finalMatrices[i];
        }
        else
        {
            DirectX::XMStoreFloat4x4(
                &boneConstants.bones[i],
                DirectX::XMMatrixIdentity());
        }
    }

    m_context->UpdateSubresource(
        m_boneConstantBuffer,
        0,
        nullptr,
        &boneConstants,
        0,
        0);

    m_context->IASetInputLayout(m_inputLayout);
    m_context->VSSetShader(m_vertexShader, nullptr, 0);
    m_context->PSSetShader(m_pixelShader, nullptr, 0);

    ID3D11Buffer* constantBuffers[] =
    {
        m_matrixConstantBuffer,
        m_boneConstantBuffer
    };
    m_context->VSSetConstantBuffers(0, 2, constantBuffers);
    m_context->PSSetConstantBuffers(0, 1, constantBuffers);   // matrices para la especular

    m_context->PSSetSamplers(0, 1, &m_sampler);
    m_context->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const UINT stride = static_cast<UINT>(sizeof(AnimatedVertex));
    const UINT offset = 0;

    for (const MeshBuffers& mesh : m_meshBuffers)
    {
        if (!mesh.vertexBuffer || !mesh.indexBuffer || mesh.indexCount == 0)
            continue;

        m_context->IASetVertexBuffers(
            0, 1, &mesh.vertexBuffer, &stride, &offset);
        m_context->IASetIndexBuffer(
            mesh.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
        m_context->PSSetShaderResources(
            0, 1, &mesh.diffuseTexture);

        m_context->DrawIndexed(mesh.indexCount, 0, 0);
    }
}

/**
Draw overload
**/
void AnimatedModel::Draw(
    const float* view,
    const float* projection,
    float terrainY,
    float rotationRadians,
    char axis,
    float scale)
{
    DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixIdentity();

    switch (axis)
    {
    case 'X':
    case 'x':
        rotationMatrix = DirectX::XMMatrixRotationX(rotationRadians);
        break;

    case 'Y':
    case 'y':
        rotationMatrix = DirectX::XMMatrixRotationY(rotationRadians);
        break;

    case 'Z':
    case 'z':
        rotationMatrix = DirectX::XMMatrixRotationZ(rotationRadians);
        break;
    }

    const DirectX::XMMATRIX scaleMatrix =
        DirectX::XMMatrixScaling(scale, scale, scale);

    const DirectX::XMMATRIX translationMatrix =
        DirectX::XMMatrixTranslation(m_posX, terrainY, m_posZ);

    const DirectX::XMMATRIX worldMatrix =
        rotationMatrix * scaleMatrix * translationMatrix;

    DirectX::XMFLOAT4X4 worldFloat;
    DirectX::XMStoreFloat4x4(&worldFloat, worldMatrix);

    Draw(view, projection, &worldFloat._11);
}


void AnimatedModel::Draw(
    const float* view,
    const float* projection,
    float terrainY,
    float yawRadians,
    float xAxisCorrection,
    float yAxisCorrection,
    float zAxisCorrection,
    float scale,
    float footOffset)
{

    const DirectX::XMMATRIX correctionMatrix =
        DirectX::XMMatrixRotationX(xAxisCorrection) *
        DirectX::XMMatrixRotationY(yAxisCorrection) *
        DirectX::XMMatrixRotationZ(zAxisCorrection);

    const DirectX::XMMATRIX yawMatrix =
        DirectX::XMMatrixRotationY(yawRadians);

    const DirectX::XMMATRIX scaleMatrix =
        DirectX::XMMatrixScaling(scale, scale, scale);

    const DirectX::XMMATRIX translationMatrix =
        DirectX::XMMatrixTranslation(
            m_posX,
            terrainY - footOffset * scale,
            m_posZ);

    const DirectX::XMMATRIX worldMatrix =
        correctionMatrix * yawMatrix * scaleMatrix * translationMatrix;

    DirectX::XMFLOAT4X4 worldFloat;
    DirectX::XMStoreFloat4x4(&worldFloat, worldMatrix);

    Draw(view, projection, &worldFloat._11);
}

namespace
{
    DirectX::XMMATRIX MatrizMundoAnimado(
        float posX,
        float posZ,
        float terrenoY,
        float yawRadians,
        float correccionX,
        float correccionY,
        float correccionZ,
        float scale,
        float footOffset)
    {
        const DirectX::XMMATRIX correctionMatrix =
            DirectX::XMMatrixRotationX(correccionX) *
            DirectX::XMMatrixRotationY(correccionY) *
            DirectX::XMMatrixRotationZ(correccionZ);

        const DirectX::XMMATRIX yawMatrix =
            DirectX::XMMatrixRotationY(yawRadians);

        const DirectX::XMMATRIX scaleMatrix =
            DirectX::XMMatrixScaling(scale, scale, scale);

        const DirectX::XMMATRIX translationMatrix =
            DirectX::XMMatrixTranslation(
                posX,
                terrenoY - footOffset * scale,
                posZ);

        return correctionMatrix * yawMatrix * scaleMatrix * translationMatrix;
    }
}

void AnimatedModel::GetDimensionesMundo(
    float correccionX,
    float correccionY,
    float correccionZ,
    float scale,
    float& ancho,
    float& alto,
    float& fondo) const
{
    ancho = 0.0f;
    alto = 0.0f;
    fondo = 0.0f;

    if (!m_tieneLimites)
        return;

    const DirectX::XMMATRIX worldMatrix = MatrizMundoAnimado(
        0.0f, 0.0f, 0.0f, 0.0f,
        correccionX, correccionY, correccionZ,
        scale, 0.0f);

    DirectX::XMVECTOR minimo =
        DirectX::XMVectorSet(1.0e30f, 1.0e30f, 1.0e30f, 1.0f);
    DirectX::XMVECTOR maximo =
        DirectX::XMVectorSet(-1.0e30f, -1.0e30f, -1.0e30f, 1.0f);

    for (int i = 0; i < 8; ++i)
    {
        DirectX::XMVECTOR esquina = DirectX::XMVectorSet(
            (i & 1) ? m_maxLocal[0] : m_minLocal[0],
            (i & 2) ? m_maxLocal[1] : m_minLocal[1],
            (i & 4) ? m_maxLocal[2] : m_minLocal[2],
            1.0f);

        const DirectX::XMVECTOR punto =
            DirectX::XMVector3TransformCoord(esquina, worldMatrix);

        minimo = DirectX::XMVectorMin(minimo, punto);
        maximo = DirectX::XMVectorMax(maximo, punto);
    }

    DirectX::XMFLOAT3 minimoFloat;
    DirectX::XMFLOAT3 maximoFloat;
    DirectX::XMStoreFloat3(&minimoFloat, minimo);
    DirectX::XMStoreFloat3(&maximoFloat, maximo);

    ancho = maximoFloat.x - minimoFloat.x;
    alto = maximoFloat.y - minimoFloat.y;
    fondo = maximoFloat.z - minimoFloat.z;
}

bool AnimatedModel::AgregarCajaColision(
    SistemaColision& sistema,
    float terrenoY,
    float yawRadians,
    float correccionX,
    float correccionY,
    float correccionZ,
    float scale,
    float footOffset,
    bool solido,
    const char* nombre) const
{
    if (!m_tieneLimites)
        return false;

    const DirectX::XMMATRIX worldMatrix = MatrizMundoAnimado(
        m_posX, m_posZ, terrenoY, yawRadians,
        correccionX, correccionY, correccionZ,
        scale, footOffset);

    DirectX::XMVECTOR minimo =
        DirectX::XMVectorSet(1.0e30f, 1.0e30f, 1.0e30f, 1.0f);
    DirectX::XMVECTOR maximo =
        DirectX::XMVectorSet(-1.0e30f, -1.0e30f, -1.0e30f, 1.0f);

    for (int i = 0; i < 8; ++i)
    {
        DirectX::XMVECTOR esquina = DirectX::XMVectorSet(
            (i & 1) ? m_maxLocal[0] : m_minLocal[0],
            (i & 2) ? m_maxLocal[1] : m_minLocal[1],
            (i & 4) ? m_maxLocal[2] : m_minLocal[2],
            1.0f);

        const DirectX::XMVECTOR punto =
            DirectX::XMVector3TransformCoord(esquina, worldMatrix);

        minimo = DirectX::XMVectorMin(minimo, punto);
        maximo = DirectX::XMVectorMax(maximo, punto);
    }

    DirectX::XMFLOAT3 minimoFloat;
    DirectX::XMFLOAT3 maximoFloat;
    DirectX::XMStoreFloat3(&minimoFloat, minimo);
    DirectX::XMStoreFloat3(&maximoFloat, maximo);

    sistema.AgregarCajaMinMax(
        D3DXVECTOR3(minimoFloat.x, minimoFloat.y, minimoFloat.z),
        D3DXVECTOR3(maximoFloat.x, maximoFloat.y, maximoFloat.z),
        solido,
        false,
        nombre);

    return true;
}

