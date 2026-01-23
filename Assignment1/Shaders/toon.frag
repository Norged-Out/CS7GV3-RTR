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
uniform float specularStr; // Specular strength
uniform float shininess; // Shininess factor
uniform int toonLevels;  // Number of toon shading bands

uniform bool enableRim; // Toggle Rim Lighting
uniform float rimStrength; // Strength of Rim Lighting


void main() {
    // Lighting Vectors
    vec3 N = normalize(normalWS);
    vec3 L = normalize(lightPos - currPos);
    vec3 V = normalize(camPos - currPos);
    vec3 H = normalize(L + V);  // Halfway vector

    // Attenuation
    float distance = length(lightPos - currPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Diffuse (quantize into discrete bands)
    float diffuseIntensity = max(dot(N, L), 0.0); 
    float levels = float(toonLevels);
    float diffuse = floor(diffuseIntensity * levels) / levels;
    
    // Specular (quantize into discrete bands)
    float spec = pow(max(dot(N, H), 0.0), shininess);
    if (spec > 0.01) spec = floor(spec * levels) / levels;
    float specular = specularStr * spec;

    // Rim Lighting
    float rim = 0.0;
    if (enableRim) {
        float rimFactor = 1.0 - max(dot(N, V), 0.0); // Edges perpendicular to camera
        float rimIntensity = pow(rimFactor, 3); // sharp falloff
        if (rimIntensity > 0.5) rim = rimStrength; // threshold application
    }
    
    // Sample textures
    vec4 diffuseColor = texture(diffuse0, texCoord * uvScale);
    float specularMap = texture(specular0, texCoord * uvScale).r;
    
    // Combine
    vec3 result = (diffuseColor.rgb * (ambient + diffuse) + specularMap * specular + rim) * lightColor.rgb;

    result *= attenuation;  // Apply distance falloff

    fragColor = vec4(result, diffuseColor.a);
}
