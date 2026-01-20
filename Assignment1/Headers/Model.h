#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include "Mesh.h"
class Shader;

class Model
{
public:
    // optional root transform
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // double constructors
    explicit Model(const std::string& path);
    Model(const std::string& path, const std::vector<std::string>& skipNames);

    // simple setters for TRS
    void setPosition(const glm::vec3& pos);
    void setRotation(float angleDeg, const glm::vec3& axis);
    void setScale(const glm::vec3& s);

    // axis-aligned bounding box (model space)
    glm::vec3 getAABBMin() const { return aabbMin; }
    glm::vec3 getAABBMax() const { return aabbMax; }
    glm::vec3 getAABBCenter() const { return (aabbMin + aabbMax) * 0.5f; }
    glm::vec3 getAABBSize() const { return (aabbMax - aabbMin); }

    // draw the model's meshes
    void Draw(Shader& shader);

private:
    // model space bounds
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());

	// the shared asset data
    std::vector<std::shared_ptr<Mesh>> meshes;

    // Skip some unwanted meshes
    std::vector<std::string> meshNameSkips;
    bool shouldSkipMesh(const std::string& name) const;

	// procedure to load model
    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    std::shared_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene);
};