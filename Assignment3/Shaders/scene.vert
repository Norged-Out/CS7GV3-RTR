#version 330 core

layout (location = 0) in vec3 aPos;     // Vertex position
layout (location = 1) in vec3 aNormal;  // Normals
layout (location = 2) in vec3 aColor;   // Vertex color
layout (location = 3) in vec2 aTex;     // Texture Coordinates

out vec3 currPos;      // Pass the current position
out vec3 normalWS;     // Pass normal to fragment shader
out vec3 vertexColor;  // Pass color to fragment shader
out vec2 texCoord;     // Pass texture coordinates to fragment shader

// Imports the camera matrix from the main function
uniform mat4 camMatrix;  // proj * view

// Imports the model matrix from the main function
uniform mat4 model;


void main() {
    // local values
    vec4 localPos = vec4(aPos, 1.0f);
    vec3 localNormal = aNormal;

    // transform into world space 
    vec4 worldPos = model * localPos;
    currPos = worldPos.xyz;

    // assign the normal from model space to world space
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    normalWS = normalize(normalMatrix * localNormal);

    // pass color and tex coords
    vertexColor = aColor;
    texCoord = aTex;

    // final clip-space position
    gl_Position = camMatrix * worldPos;
}