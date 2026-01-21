#version 330 core

in vec3 currPos;       // Receive the current position
in vec3 normalWS;		// Receive world space normal
in vec3 vertexColor;   // Receive color from vertex shader
in vec2 texCoord;      // Receive texture coordinates from vertex shader

out vec4 fragColor;

uniform sampler2D diffuse0; // texture unit for diffuse
uniform sampler2D specular0; // texture unit for specular
uniform float uvScale = 1.0;

uniform vec4 lightColor; // Gets the color of the light
uniform vec3 lightDir;   // Gets the direction of the light

uniform float ambient; // Ambient strength
uniform float specularStr; // Specular strength
uniform float shininess; // Shininess factor


uniform vec3 camPos; // Gets the position of the camera


vec4 directLight() {
    // Lighting Vectors
    vec3 N = normalize(normalWS);
    vec3 L = normalize(lightDir);
    vec3 V = normalize(camPos - currPos);
    vec3 H = normalize(L + V);  // Halfway vector for Blinn-Phong
    
    // Diffuse
    float diffuse = max(dot(N, L), 0.0);
    
    // Specular (Blinn-Phong using halfway vector)
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float specular = specularStr * spec;
    
    // Sample textures
    vec4 diffuseColor = texture(diffuse0, texCoord * uvScale);
    float specularMap = texture(specular0, texCoord * uvScale).r;
    
    // Combine
    return (diffuseColor * (ambient + diffuse) + specularMap * specular) * lightColor;
}


void main() {
	fragColor = directLight();
}
