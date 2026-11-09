#pragma once
#include <string>

class FBXImporter
{
public:
    bool Load(const std::string& filename, ImportedModel& output);

private:
    void ProcessNode(
        aiNode* node,
        const aiScene* scene,
        ImportedModel& output);

    void ProcessMesh(
        aiMesh* mesh,
        const aiScene* scene,
        ImportedModel& output);

    void ProcessBones(
        aiMesh* mesh,
        ImportedModel& output);

    void ProcessAnimations(
        const aiScene* scene,
        ImportedModel& output);
};
