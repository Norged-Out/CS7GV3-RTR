#version 330 core

in vec3 currPos;       // Receive the current position
in vec3 normalWS;	   // Receive world space normal
in vec3 vertexColor;   // Receive color from vertex shader
in vec2 texCoord;      // Receive texture coordinates from vertex shader
in mat3 TBN;           // Receive tangent-space basis from vertex shader

out vec4 fragColor;

uniform bool useTextures = false; // Toggle texture usage
uniform bool useNormalMap = false; // Toggle normal mapping

uniform sampler2D diffuse0; // texture unit for diffuse
uniform sampler2D specular0; // texture unit for specular
uniform sampler2D normal0; // normal map

uniform float uvScale = 1.0;

uniform vec4 lightColor; // Gets the color of the light
uniform vec3 lightPos;   // Gets the position of the light
uniform vec3 camPos; // Gets the position of the camera

uniform float ambient; // Ambient strength
uniform float specularStr = 5.0f; // Specular strength
uniform float shininess = 32.0f; // Shininess factor


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
        
    // Diffuse
    float diffuse = max(dot(N, L), 0.0);
    
    // Specular (Blinn-Phong using halfway vector)
    float spec = pow(max(dot(N, H), 0.0), shininess);
    float specular = specularStr * spec;
    
    // Texture sampling
    vec4 baseColor = useTextures
        ? texture(diffuse0, texCoord * uvScale)
        : vec4(vertexColor, 1.0);

    float specularMap = useTextures
        ? texture(specular0, texCoord * uvScale).r
        : 0.5;
    
    // Combine
    vec3 result = (baseColor.rgb * (ambient + diffuse) + specularMap * specular) * lightColor.rgb;

    result *= attenuation;  // Apply distance falloff

    fragColor = vec4(result, baseColor.a);
    //fragColor = vec4(normalize(N) * 0.5 + 0.5, 1.0);
}
