#include "Model.h"
#include "Shader.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/texture.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Constructor to load model
Model::Model(const std::string& path) {
    loadModel(path);
}

// alt Constructor for funky stuff
Model::Model(const std::string& path, const std::vector<std::string>& skipNames)
    : meshNameSkips(skipNames){
    loadModel(path);
}

void Model::setPosition(const glm::vec3& pos) {
    // overwrite translation component, preserve existing rotation/scale
    modelMatrix[3] = glm::vec4(pos, 1.0f);
}

void Model::setRotation(float angleDeg, const glm::vec3& axis) {
    // apply rotation on top of current matrix
    modelMatrix = glm::rotate(modelMatrix, glm::radians(angleDeg), axis);
}

void Model::setScale(const glm::vec3& s) {
    // apply scale on top of current matrix
    modelMatrix = glm::scale(modelMatrix, s);
}


void Model::Draw(Shader& shader) {
    if (meshes.empty()) return; // guard
    // draws each mesh onto scene
    for (auto& mesh : meshes) {
        // combine model transform with mesh
        glm::mat4 finalMatrix = modelMatrix * mesh->getModelMatrix();
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

static void AttachEmbeddedTextures(std::vector<Texture>& textures,
    aiMaterial* material, const aiScene* scene) {
    aiString texPath;
    bool attached = false;

    auto attach = [&](aiTextureType t) {
        if (material->GetTextureCount(t) == 0) return false;
        if (material->GetTexture(t, 0, &texPath) != AI_SUCCESS) return false;

        // Embedded textures have paths like "*0"
        if (texPath.length > 0 && texPath.C_Str()[0] == '*') {
            int idx = std::atoi(texPath.C_Str() + 1);
            const aiTexture* tex = scene->mTextures[idx];
            if (!tex) return false;

            // Common GLB case: compressed image data (mHeight == 0)
            if (tex->mHeight == 0) {
                const unsigned char* bytes = reinterpret_cast<const unsigned char*>(tex->pcData);
                size_t size = tex->mWidth; // byte size when compressed
                // diffuse0 on texture unit 0
                textures.emplace_back(bytes, size, "diffuse", 0, GL_UNSIGNED_BYTE);
                // specular0 on texture unit 1 (simple mirror so shader has something bound)
                textures.emplace_back(bytes, size, "specular", 1, GL_UNSIGNED_BYTE);
                return true;
            }

            // If it's uncompressed BGRA (mHeight != 0), skip for now
            return false;
        }

        // External file case (rare for .glb) — you can wire later if you want
        return false;
        };

    // Try BASE_COLOR first, then DIFFUSE
    attached = attach(aiTextureType_BASE_COLOR) || attach(aiTextureType_DIFFUSE);
}


std::shared_ptr<Mesh> Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;

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
        AttachEmbeddedTextures(textures, material, scene);
    }

    // construct Mesh in place once and transfer ownership into Model
    return std::make_shared<Mesh>(vertices, indices, textures);
}