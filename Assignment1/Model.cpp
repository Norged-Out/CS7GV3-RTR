#include "Model.h"
#include "Shader.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/texture.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


// helper to extract model path
static std::string getModelDirectory(const std::string& modelPath) {
    size_t lastSlash = modelPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return modelPath.substr(0, lastSlash + 1);
    }
    return "";  // Model in current directory
}

// Constructor to load model
Model::Model(const std::string& path) {
    loadModel(path);
}

// alt Constructor for funky stuff
Model::Model(const std::string& path, const std::vector<std::string>& skipNames)
    : meshNameSkips(skipNames){
    loadModel(path);
}

// alt Constructor to force diffuse/specular textures
Model::Model(const std::string& path,
    const std::string& diffusePath,
    const std::string& specularPath)
    : diffusePath(diffusePath), specularPath(specularPath) {
    loadModel(path);
}


void Model::setPosition(const glm::vec3& pos) { position = pos; }

void Model::setRotation(float angleDeg, const glm::vec3& axis) {
    rotation = glm::angleAxis(glm::radians(angleDeg), glm::normalize(axis));
}

void Model::setScale(const glm::vec3& s) { scale = s; }


void Model::Draw(Shader& shader) {
    if (meshes.empty()) return; // guard
    glm::mat4 computedMatrix = getModelMatrix();  // Compute TRS from components
    // draws each mesh onto scene
    for (auto& mesh : meshes) {
        // combine model transform with mesh
        glm::mat4 finalMatrix = computedMatrix * mesh->getModelMatrix();
        // export the finalMatrix to the Vertex Shader of model
        shader.setMat4("model", finalMatrix);
        // issue the actual draw for this mesh
        mesh->Draw(shader);
    }
}

void Model::loadModel(const std::string& path) {
    // create Assimp importer
    Assimp::Importer importer;

    // import flags
    unsigned int flags =
        aiProcess_Triangulate           | // Ensures all faces are triangles
        aiProcess_GenNormals            | // Generates normals if missing
        aiProcess_JoinIdenticalVertices |  // Optimizes geometry
        aiProcess_PreTransformVertices  | // Bake node transforms into vertices
        aiProcess_OptimizeMeshes; // Merge tiny meshes to reduce draw calls

    // import the 3D model file
    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
        return;
    }

    // begin recursively processing the model hierarchy
    processNode(scene->mRootNode, scene);
}


bool Model::shouldSkipMesh(const std::string& name) const {
    for (const auto& s : meshNameSkips) {
        if (name.find(s) != std::string::npos) return true;
    }
    return false;
}

void Model::processNode(aiNode* node, const aiScene* scene) {
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

        // Debug: print mesh info
        std::string meshName = mesh->mName.C_Str();
        std::cout << "[Model] Mesh: " << meshName
            << " | Vertices: " << mesh->mNumVertices << std::endl;
;
        if (shouldSkipMesh(meshName)) {
            std::cout << "[Model] Skipping mesh: " << meshName << std::endl;
            continue;
        }

        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            const aiVector3D& v = mesh->mVertices[i];
            // expand model-space AABB
            aabbMin.x = std::min(aabbMin.x, v.x);
            aabbMin.y = std::min(aabbMin.y, v.y);
            aabbMin.z = std::min(aabbMin.z, v.z);
            aabbMax.x = std::max(aabbMax.x, v.x);
            aabbMax.y = std::max(aabbMax.y, v.y);
            aabbMax.z = std::max(aabbMax.z, v.z);
        }

        // store mesh
        meshes.emplace_back(processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Model::AttachTextures(std::vector<std::shared_ptr<Texture>>& textures,
    aiMaterial* material, const aiScene* scene) {

    // try Embedded textures first
    auto loadTexture = [&](aiTextureType aiType, const char* typeName, GLuint slot) -> bool {
        if (material->GetTextureCount(aiType) == 0) return false;

        aiString texPath;
        if (material->GetTexture(aiType, 0, &texPath) != AI_SUCCESS) return false;
        std::cout << "[DEBUG] Raw texture path: \"" << texPath.C_Str() << "\"\n";

        // Embedded texture 
        if (texPath.length > 0 && texPath.C_Str()[0] == '*') {
            int idx = std::atoi(texPath.C_Str() + 1);
            if (idx >= 0 && idx < (int)scene->mNumTextures) {
                const aiTexture* tex = scene->mTextures[idx];
                if (!tex) return false;

                // Compressed embedded texture (GLB case)
                if (tex->mHeight == 0) {
                    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(tex->pcData);
                    size_t size = tex->mWidth;
                    textures.emplace_back(std::make_shared<Texture>(bytes, size, typeName, slot, GL_UNSIGNED_BYTE));
                    std::cout << "[Texture] Loaded embedded " << typeName << "\n";
                    return true;
                }
            }
        }
        return false;
        };

    // Try to load diffuse texture (slot 0)
    bool hasDiffuse = loadTexture(aiTextureType_BASE_COLOR, "diffuse", 0) ||
                      loadTexture(aiTextureType_DIFFUSE, "diffuse", 0);

    // Try to load specular texture (slot 1)
    bool hasSpecular = loadTexture(aiTextureType_SPECULAR, "specular", 1);

	// attempt manual override paths if provided
    if (!hasDiffuse && !diffusePath.empty()) {
        try {
            textures.emplace_back(std::make_shared<Texture>(
                diffusePath.c_str(), "diffuse", 0, GL_UNSIGNED_BYTE));
            std::cout << "[Texture] Loaded manual diffuse: " << diffusePath << "\n";
            hasDiffuse = true;
        }
        catch (...) {
            std::cerr << "[Texture] Failed to load manual diffuse\n";
        }
    }

    if (!hasSpecular && !specularPath.empty()) {
        try {
            textures.emplace_back(std::make_shared<Texture>(
                specularPath.c_str(), "specular", 1, GL_UNSIGNED_BYTE));
            std::cout << "[Texture] Loaded manual specular: " << specularPath << "\n";
            hasSpecular = true;
        }
        catch (...) {
            std::cerr << "[Texture] Failed to load manual specular\n";
        }
    }


    if (!hasDiffuse) {
        std::cout << "[Texture] Warning: No diffuse texture found\n";
    }
    if (!hasSpecular) {
        std::cout << "[Texture] Note: No specular texture found\n";
    }
}



std::shared_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<std::shared_ptr<Texture>> textures;

    // extract vertex data
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
        // Vertex position
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        // Normals (if they exist)
        if (mesh->HasNormals())
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        else
            vertex.normal = glm::vec3(0.0f);

        // Texture coordinates
        if (mesh->HasTextureCoords(0))
            vertex.texUV = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.texUV = glm::vec2(0.0f);

        // Optional: Vertex color
        vertex.color = glm::vec3(1.0f); // Default white

        vertices.push_back(vertex);
    }

    // process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(static_cast<GLuint>(face.mIndices[j]));
        }
    }

    // process textures
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        AttachTextures(textures, material, scene);
    }

    // construct Mesh in place once and transfer ownership into Model
    return std::make_shared<Mesh>(vertices, indices, textures);
}