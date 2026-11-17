#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>

#include "Colision.h"

class FBXImporter;
struct AnimationClip;

struct MeshBuffers
{
    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    UINT indexCount = 0;
    ID3D11ShaderResourceView* diffuseTexture = nullptr;
};

class AnimatedModel
{
public:
    // Matrix world position
    AnimatedModel(ID3D11Device* device, ID3D11DeviceContext* context);

    // overload with world coordinates provided
    AnimatedModel(
        ID3D11Device* device,
        ID3D11DeviceContext* context,
        float positionX,
        float positionZ);

    ~AnimatedModel();

    bool Load(
        const std::string& filepath,
        const std::string& diffuseTextureOverride = {});
    bool LoadAnimationFile(
        const std::string& filepath,
        const std::string& clipName);
    void PlayAnimation(const std::string& name, bool loop = true);
    void Update(float deltaTime);
    bool IsAnimationFinished() const { return m_animationFinished; }

    float GetPosX() const { return m_posX; }
    float GetPosZ() const { return m_posZ; }

    void SetPosition(float x, float z)
    {
        m_posX = x;
        m_posZ = z;
    }

    // Limites de la malla en el espacio local del modelo (pose de bind).
    bool TieneLimitesLocales() const { return m_tieneLimites; }

    void GetLimitesLocales(
        float& minX, float& minY, float& minZ,
        float& maxX, float& maxY, float& maxZ) const
    {
        minX = m_minLocal[0]; minY = m_minLocal[1]; minZ = m_minLocal[2];
        maxX = m_maxLocal[0]; maxY = m_maxLocal[1]; maxZ = m_maxLocal[2];
    }

    // Dimensiones (ancho, alto, fondo) de la malla despues de aplicar las
    // mismas rotaciones de correccion y escala que usa Draw.
    void GetDimensionesMundo(
        float correccionX,
        float correccionY,
        float correccionZ,
        float scale,
        float& ancho,
        float& alto,
        float& fondo) const;

    // Caja que encierra al personaje en el mundo, con la misma
    // transformacion que el Draw con correcciones.
    bool AgregarCajaColision(
        SistemaColision& sistema,
        float terrenoY,
        float yawRadians,
        float correccionX,
        float correccionY,
        float correccionZ,
        float scale,
        float footOffset,
        bool solido = true,
        const char* nombre = nullptr) const;


    // Draw with an already built world matrix.
    void Draw(const float* view, const float* projection, const float* world);
    
    /// Draw at the stored X/Z position and terrain height.
    // rotationRadians is in radians; axis is 'X', 'Y', or 'Z'.
    void Draw(
        const float* view,
        const float* projection,
        float terrainY,
        float rotationRadians,
        char axis,
        float scale);

/// <summary>
/// Draw function to correct 3d model orientation
/// </summary>
    void Draw(
        const float* view,
        const float* projection,
        float terrainY,
        float yawRadians,
        float xAxisCorrection,
        float yAxisCorrection,
        float zAxisCorrection,
        float scale,
        float footOffset);

private:
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;

    std::unique_ptr<FBXImporter> m_importer;
    std::vector<MeshBuffers> m_meshBuffers;

    ID3D11VertexShader* m_vertexShader = nullptr;
    ID3D11PixelShader* m_pixelShader = nullptr;
    ID3D11InputLayout* m_inputLayout = nullptr;
    ID3D11Buffer* m_boneConstantBuffer = nullptr;
    ID3D11Buffer* m_matrixConstantBuffer = nullptr;
    ID3D11SamplerState* m_sampler = nullptr;

    float m_posX = 0.0f;
    float m_posZ = 0.0f;

    float m_minLocal[3] = { 0.0f, 0.0f, 0.0f };
    float m_maxLocal[3] = { 0.0f, 0.0f, 0.0f };
    bool m_tieneLimites = false;

    float m_globalInverseRoot[16] = {};
    std::string m_currentAnimation;
    float m_animationTime = 0.0f;
    bool m_loopAnimation = true;
    bool m_animationFinished = false;

    void CreateShadersAndInputLayout();
    void TraverseHierarchy(
        int nodeIndex,
        const float* parentMatrix,
        const AnimationClip* clip);
};
