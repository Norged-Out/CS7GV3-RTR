#version 330 core

layout (location = 0) in vec3 aPos; // Vertex position

out float shadowDepth; // Pass shadow-space depth to fragment shader

// Model matrix from the mesh
uniform mat4 model;

// Light projection * view
uniform mat4 lightSpaceMatrix;

void main()
{
    // Transform vertex into light clip space
    vec4 lightClipPos = lightSpaceMatrix * model * vec4(aPos, 1.0);
    gl_Position = lightClipPos;

    // Shadow-space depth remapped to [0, 1] for both depth and MSM paths.
    shadowDepth = lightClipPos.z / lightClipPos.w;
    shadowDepth = shadowDepth * 0.5 + 0.5;
}
