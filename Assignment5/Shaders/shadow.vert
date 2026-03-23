#version 330 core

layout (location = 0) in vec3 aPos;

// Model matrix from the mesh
uniform mat4 model;

// Light projection * view
uniform mat4 lightSpaceMatrix;

void main()
{
    // Transform vertex into light clip space
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}