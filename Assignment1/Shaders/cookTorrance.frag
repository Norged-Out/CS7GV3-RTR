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
uniform vec3 lightPos;   // Gets the position of the light
uniform vec3 camPos; // Gets the position of the camera

uniform float ambient; // Ambient strength
uniform float metallic; // Metalness factor
uniform float roughness; // Surface roughness

const float PI = 3.14159265359;

// GGX Distribution that controls shape of highlights
// Rough surface = wide and dim highlights
// Smooth surface = tight and bright highlights
float GGXDistribution(float NdotH, float roughness) {
    float a = roughness * roughness; // linear
    float a2 = a * a;
    float denom = (NdotH * NdotH) * (a2 - 1.0) + 1.0; // Bell Curve denominator
    denom = PI * denom * denom;
    return a2 / denom; // D term for alligned microfacets
}

// Shlick Fresnel function
vec3 FresnelSchlick(float cosTheta, vec3 F0) {

}

void main() {
    // Lighting Vectors
    vec3 N = normalize(normalWS);
    vec3 L = normalize(lightPos - currPos);
    vec3 V = normalize(camPos - currPos);
    vec3 H = normalize(L + V);  // Halfway vector for Blinn-Phong

    // Attenuation
    float distance = length(lightPos - currPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Diffuse
    float diffuse = max(dot(N, L), 0.0);
    
    // Specular (Blinn-Phong using halfway vector)
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float specular = specularStr * spec;
    
    // Sample textures
    vec4 diffuseColor = texture(diffuse0, texCoord * uvScale);
    float specularMap = texture(specular0, texCoord * uvScale).r;
    
    // Combine
    vec3 result = (diffuseColor.rgb * (ambient + diffuse) + specularMap * specular) * lightColor.rgb;

    result *= attenuation;  // Apply distance falloff

    fragColor = vec4(result, diffuseColor.a);
}
