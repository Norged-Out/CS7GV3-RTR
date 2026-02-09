#version 330 core

layout (location = 0) in vec3 aPos;     // Vertex position
layout (location = 1) in vec3 aNormal;  // Normals
layout (location = 2) in vec3 aColor;   // Vertex color
layout (location = 3) in vec2 aTex;     // Texture Coordinates
layout (location = 4) in vec3 aTangent; // Tangent for normal mapping

out vec3 currPos;      // Pass the current position
out vec3 normalWS;     // Pass normal to fragment shader
out vec3 vertexColor;  // Pass color to fragment shader
out vec2 texCoord;     // Pass texture coordinates to fragment shader
out mat3 TBN;          // Pass tangent-space basis to fragment shader

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

    // transform tangent into world space
    vec3 tangent = normalize(normalMatrix * aTangent);

    // re-orthogonalize tangent with respect to the normal
    tangent = normalize(tangent - normalWS * dot(normalWS, tangent));

    // derive bitangent from normal and tangent
    vec3 bitangent = normalize(cross(normalWS, tangent));

    // tangent-space -> world-space basis matrix
    TBN = mat3(tangent, bitangent, normalWS);

    // final clip-space position
    gl_Position = camMatrix * worldPos;
}