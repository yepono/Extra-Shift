#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <assimp/scene.h>
#include "AnimatedVertex.h"
#include "Skeleton.h"
#include "AnimationClip.h"


struct SubMesh {
    std::vector<AnimatedVertex> vertices;
    std::vector<unsigned int> indices;
    std::string diffuseTexturePath;
    std::string normalTexturePath;
    std::string specularTexturePath;
};

class FBXImporter {
public:
    bool Load(const std::string& filepath);

    std::vector<SubMesh> subMeshes;
    Skeleton skeleton;
    std::unordered_map<std::string, AnimationClip> animations;
    std::string directory;
    
    bool LoadAnimationFile(
        const std::string& filepath,
        const std::string& clipName);

private:
    void ExtractBoneWeights(const aiMesh* mesh, std::vector<AnimatedVertex>& vertices, Skeleton& skeleton);
    int FindBoneIndex(Skeleton& skeleton, const std::string& name);
    int ParseNodeHierarchy(const aiNode* node, int parentIndex, Skeleton& skeleton);
};
