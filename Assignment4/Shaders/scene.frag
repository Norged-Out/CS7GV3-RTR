#version 330 core

in vec3 currPos;       // Receive the current position
in vec3 normalWS;	   // Receive world space normal
in vec3 vertexColor;   // Receive color from vertex shader
in vec2 texCoord;      // Receive texture coordinates from vertex shader
in mat3 TBN;           // Receive tangent-space basis from vertex shader

out vec4 fragColor;

uniform bool useTextures = true; // Toggle texture usage
uniform bool useNormalMap = true; // Toggle normal mapping

uniform sampler2D diffuse0; // texture unit for diffuse
uniform sampler2D normal0; // normal map
uniform sampler2D roughness0; // texture unit for roughness

uniform float uvScale = 1.0;

uniform vec4 lightColor; // Gets the color of the light
uniform vec3 lightPos;   // Gets the position of the light
uniform vec3 camPos; // Gets the position of the camera

uniform float ambient; // Ambient strength
uniform float specularStr = 2.5f; // Specular strength
uniform float roughnessBias = 0.0f; // Bias to adjust roughness


void main() {
    // Normal selection 
    vec3 N = normalize(normalWS);
    if (useNormalMap) {
        // Sample tangent-space normal from texture
        vec3 normalTS = texture(normal0, texCoord * uvScale).rgb;
        // Unpack from [0,1] to [-1,1]
        normalTS = normalTS * 2.0 - 1.0;
        // DirectX normal map fix
        normalTS.y = -normalTS.y;
        // Transform normal from tangent space to world space
        N = normalize(TBN * normalTS);
    }

    // Lighting Vectors
    vec3 L = normalize(lightPos - currPos);
    vec3 V = normalize(camPos - currPos);
    vec3 H = normalize(L + V);  // Halfway vector for Blinn-Phong

    // Attenuation
    float distance = length(lightPos - currPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
    // Base Color
    vec3 baseColor = useTextures
        ? texture(diffuse0, texCoord * uvScale).rgb
        : vertexColor;

    // Diffuse component
    vec3 diffuse = baseColor * max(dot(N, L), 0.0) * lightColor.rgb;

    // Roughness-driven specular
    float roughness = useTextures
        ? texture(roughness0, texCoord * uvScale).r
        : 0.5;

    roughness = clamp(roughness + roughnessBias, 0.0, 1.0);
    float smoothness = 1.0 - roughness;

    // map roughness -> shininess
    float shininess = mix(8.0, 128.0, smoothness);
    // Blinn-Phong specular
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float specular = spec * specularStr * smoothness;
    
    // Combine
    vec3 result = baseColor * ambient;
    result += (diffuse + specular * lightColor.rgb) * attenuation; // Apply distance falloff

    fragColor = vec4(result, 1.0);
}
