#pragma once

#include <string>
#include <glad/glad.h>

// Loads an HDR equirectangular texture from disk
class EXR {
public:
    GLuint ID;
    int width = 0;
    int height = 0;

    EXR(const std::string& path);
    void Bind(GLuint unit = 0) const;
};
