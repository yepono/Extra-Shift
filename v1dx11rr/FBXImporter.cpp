/*aiProcess_Triangulate
aiProcess_GenSmoothNormals
aiProcess_CalcTangentSpace
aiProcess_JoinIdenticalVertices
aiProcess_LimitBoneWeights
aiProcess_ConvertToLeftHanded*/

#include "FBXImporter.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <string>


bool FBXImporter::Load(const std::string& filepath) {
    std::filesystem::path path(filepath);
    directory = path.parent_path().string() + "/";

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath,
        aiProcess_Triangulate |
        aiProcess_SortByPType |
        aiProcess_GenSmoothNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // Import Diagnostics for debuggin
    std::cout << "[Import Diagnostics]\n"
        << "FBX Path: " << filepath << "\n"
        << "Mesh Count: " << scene->mNumMeshes << "\n"
        << "Material Count: " << scene->mNumMaterials << "\n"
        << "Animation Count: " << scene->mNumAnimations << std::endl;

    // Parse Node Hierarchy
    ParseNodeHierarchy(scene->mRootNode, -1, skeleton);

    // Extract Meshes & Bones
    subMeshes.resize(scene->mNumMeshes);
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        auto& subMesh = subMeshes[i];

        subMesh.vertices.resize(mesh->mNumVertices);
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            subMesh.vertices[v].Position = { mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z };
            if (mesh->HasNormals()) {
                subMesh.vertices[v].Normal = { mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z };
            }
            if (mesh->HasTextureCoords(0)) {
                subMesh.vertices[v].UV = { mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y };
            }
        }

        // Extract Indices
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                subMesh.indices.push_back(face.mIndices[j]);
            }
        }

        // Extract Bone Weights
        ExtractBoneWeights(mesh, subMesh.vertices, skeleton);

        // Extract Materials/Textures
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiString texPath;
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                subMesh.diffuseTexturePath = texPath.C_Str();
            if (mat->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
                subMesh.normalTexturePath = texPath.C_Str();
            if (mat->GetTexture(aiTextureType_SPECULAR, 0, &texPath) == AI_SUCCESS)
                subMesh.specularTexturePath = texPath.C_Str();
        }
    }

    // Extract Animations
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        aiAnimation* anim = scene->mAnimations[i];
        AnimationClip clip;
        clip.name = anim->mName.C_Str();
        if (clip.name.empty()) clip.name = "DefaultAnim_" + std::to_string(i);
        clip.duration = anim->mDuration;
        clip.ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0;

        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            aiNodeAnim* nodeAnim = anim->mChannels[c];
            BoneAnimationChannel channel;
            channel.boneName = nodeAnim->mNodeName.C_Str();

            for (unsigned int p = 0; p < nodeAnim->mNumPositionKeys; ++p) {
                channel.positionKeys.push_back({ nodeAnim->mPositionKeys[p].mTime,
                    { nodeAnim->mPositionKeys[p].mValue.x, nodeAnim->mPositionKeys[p].mValue.y, nodeAnim->mPositionKeys[p].mValue.z } });
            }
            for (unsigned int r = 0; r < nodeAnim->mNumRotationKeys; ++r) {
                channel.rotationKeys.push_back({ nodeAnim->mRotationKeys[r].mTime,
                    { nodeAnim->mRotationKeys[r].mValue.x, nodeAnim->mRotationKeys[r].mValue.y, nodeAnim->mRotationKeys[r].mValue.z, nodeAnim->mRotationKeys[r].mValue.w } });
            }
            for (unsigned int s = 0; s < nodeAnim->mNumScalingKeys; ++s) {
                channel.scalingKeys.push_back({ nodeAnim->mScalingKeys[s].mTime,
                    { nodeAnim->mScalingKeys[s].mValue.x, nodeAnim->mScalingKeys[s].mValue.y, nodeAnim->mScalingKeys[s].mValue.z } });
            }
            clip.channels.push_back(channel);
        }
        animations[clip.name] = clip;

        std::cout << "[Animation Diagnostics] Name: " << clip.name
            << " | Duration: " << clip.duration
            << " | Ticks/s: " << clip.ticksPerSecond
            << " | Channels: " << clip.channels.size() << std::endl;
    }

    skeleton.finalMatrices.resize(skeleton.bones.size(), DirectX::XMFLOAT4X4(
        1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1
    ));

    return true;
}

bool FBXImporter::LoadAnimationFile(
    const std::string& filepath,
    const std::string& clipName)
{
    if (filepath.empty() || clipName.empty() || skeleton.nodes.empty())
        return false;

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filepath,
        aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene ||
        (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
        scene->mNumAnimations == 0)
    {
        std::cerr << "Animation import failed for " << filepath
            << ": " << importer.GetErrorString() << std::endl;
        return false;
    }

    const aiAnimation* source = scene->mAnimations[0];

    AnimationClip clip;
    clip.name = clipName;
    clip.duration = source->mDuration;
    clip.ticksPerSecond =
        source->mTicksPerSecond != 0.0
        ? source->mTicksPerSecond
        : 25.0;

    bool matchedSkeletonNode = false;

    for (unsigned int channelIndex = 0;
        channelIndex < source->mNumChannels;
        ++channelIndex)
    {
        const aiNodeAnim* sourceChannel = source->mChannels[channelIndex];

        BoneAnimationChannel channel;
        channel.boneName = sourceChannel->mNodeName.C_Str();

        for (const SkeletonNode& node : skeleton.nodes)
        {
            if (node.name == channel.boneName)
            {
                matchedSkeletonNode = true;
                break;
            }
        }

        for (unsigned int i = 0;
            i < sourceChannel->mNumPositionKeys;
            ++i)
        {
            const aiVectorKey& key = sourceChannel->mPositionKeys[i];
            channel.positionKeys.push_back({
                key.mTime,
                { key.mValue.x, key.mValue.y, key.mValue.z }
                });
        }

        for (unsigned int i = 0;
            i < sourceChannel->mNumRotationKeys;
            ++i)
        {
            const aiQuatKey& key = sourceChannel->mRotationKeys[i];
            channel.rotationKeys.push_back({
                key.mTime,
                {
                    key.mValue.x,
                    key.mValue.y,
                    key.mValue.z,
                    key.mValue.w
                }
                });
        }

        for (unsigned int i = 0;
            i < sourceChannel->mNumScalingKeys;
            ++i)
        {
            const aiVectorKey& key = sourceChannel->mScalingKeys[i];
            channel.scalingKeys.push_back({
                key.mTime,
                { key.mValue.x, key.mValue.y, key.mValue.z }
                });
        }

        clip.channels.push_back(channel);
    }

    if (!matchedSkeletonNode)
    {
        std::cerr << "Animation '" << clipName
            << "' has no channels matching the model skeleton." << std::endl;
        return false;
    }

    animations[clipName] = clip;

    std::cout << "Loaded animation '" << clipName
        << "' from " << filepath
        << " with " << clip.channels.size()
        << " channels." << std::endl;

    return true;
}

int FBXImporter::ParseNodeHierarchy(const aiNode* node, int parentIndex, Skeleton& skeleton) {
    SkeletonNode sNode;
    sNode.name = node->mName.C_Str();
    sNode.parentIndex = parentIndex;

    aiMatrix4x4 m = node->mTransformation;
    DirectX::XMFLOAT4X4 mat(
        m.a1, m.a2, m.a3, m.a4,
        m.b1, m.b2, m.b3, m.b4,
        m.c1, m.c2, m.c3, m.c4,
        m.d1, m.d2, m.d3, m.d4
    );
    // Assimp matrices are row-major, transpose for DirectXMath if needed depending on your math style
    DirectX::XMMATRIX xm = DirectX::XMLoadFloat4x4(&mat);
    DirectX::XMStoreFloat4x4(&sNode.localTransform, DirectX::XMMatrixTranspose(xm));

    int currentIndex = static_cast<int>(skeleton.nodes.size());
    skeleton.nodes.push_back(sNode);

    if (parentIndex != -1) {
        skeleton.nodes[parentIndex].children.push_back(currentIndex);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ParseNodeHierarchy(node->mChildren[i], currentIndex, skeleton);
    }
    return currentIndex;
}

void FBXImporter::ExtractBoneWeights(const aiMesh* mesh, std::vector<AnimatedVertex>& vertices, Skeleton& skeleton) {
    for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
        aiBone* bone = mesh->mBones[i];
        std::string boneName = bone->mName.C_Str();
        int boneIndex = FindBoneIndex(skeleton, boneName);

        if (boneIndex == -1) {
            BoneInfo bInfo;
            bInfo.name = boneName;
            aiMatrix4x4 m = bone->mOffsetMatrix;
            DirectX::XMFLOAT4X4 mat(
                m.a1, m.a2, m.a3, m.a4,
                m.b1, m.b2, m.b3, m.b4,
                m.c1, m.c2, m.c3, m.c4,
                m.d1, m.d2, m.d3, m.d4
            );
            DirectX::XMMATRIX xm = DirectX::XMLoadFloat4x4(&mat);
            DirectX::XMStoreFloat4x4(&bInfo.offsetMatrix, DirectX::XMMatrixTranspose(xm));
            boneIndex = static_cast<int>(skeleton.bones.size());
            skeleton.bones.push_back(bInfo);
        }

        for (unsigned int j = 0; j < bone->mNumWeights; ++j) {
            unsigned int vertexId = bone->mWeights[j].mVertexId;
            float weight = bone->mWeights[j].mWeight;

            // Assign weights to the vertex
            AnimatedVertex& v = vertices[vertexId];
            for (int w = 0; w < 4; ++w) {
                if (v.Weights[w] == 0.0f) {
                    v.Weights[w] = weight;
                    v.BoneIndices[w] = static_cast<unsigned int>(boneIndex);
                    break;
                }
            }
        }
    }

    // Normalize weights
    for (auto& v : vertices) {
        float sum = v.Weights[0] + v.Weights[1] + v.Weights[2] + v.Weights[3];
        if (sum > 0.0f) {
            for (int w = 0; w < 4; ++w) v.Weights[w] /= sum;
        }
    }
}

int FBXImporter::FindBoneIndex(Skeleton& skeleton, const std::string& name) {
    for (size_t i = 0; i < skeleton.bones.size(); ++i) {
        if (skeleton.bones[i].name == name) return static_cast<int>(i);
    }
    return -1;
}