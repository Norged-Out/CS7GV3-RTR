#pragma once

#include "Shader.h"
#include "Cubemap.h"
#include "Camera.h"

class Skybox {
public:
    Skybox(Cubemap& cubemap);
    ~Skybox();

    void Draw(const Camera& camera);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;

    Shader shader;
    Cubemap& environment;

    void initCube();
};
