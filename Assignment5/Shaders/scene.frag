#version 330 core

in vec3 currPos;       // Receive the current position
in vec3 normalWS;	   // Receive world space normal
in vec3 vertexColor;   // Receive color from vertex shader
in vec2 texCoord;      // Receive texture coordinates from vertex shader
in mat3 TBN;           // Receive tangent-space basis from vertex shader
in vec4 lightSpacePos;

out vec4 fragColor;

uniform bool useTextures = false; // Toggle texture usage
uniform bool useNormalMap = false; // Toggle normal mapping
uniform bool usePCF = false;

uniform sampler2D diffuse0; // texture unit for diffuse
uniform sampler2D normal0; // normal map
uniform sampler2D roughness0; // texture unit for roughness
uniform sampler2D shadowMap;

uniform float uvScale = 1.0;

uniform vec4 lightColor; // Gets the color of the light
uniform vec3 lightDir;   // Direction the light travels in world space
uniform vec3 camPos; // Gets the position of the camera

uniform float ambient; // Ambient strength
uniform float specularStr = 2.5f; // Specular strength
uniform float roughnessBias = 0.0f; // Bias to adjust roughness
uniform float normalStrength = 1.0f; // Strength of normal mapping

vec3 getShadowProjCoords() {
    // Convert clip-space position to normalized device coordinates
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Convert from [-1,1] to [0,1] coordinates
    projCoords = projCoords * 0.5 + 0.5;
    return projCoords;
}

float computeShadowBias(vec3 geomNormal, vec3 L) {
    float ndotl = max(dot(normalize(geomNormal), normalize(L)), 0.0);
    return max(0.01 * (1.0 - ndotl), 0.001);
}

float computeShadowHard(vec3 geomNormal, vec3 L) {
    vec3 projCoords = getShadowProjCoords();

    // Skip fragments outside the light frustum
    if(projCoords.z > 1.0) return 0.0;
    if(projCoords.x < 0.0 || projCoords.x > 1.0 ||
       projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    // Depth stored in shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Depth of the current fragment from the light's perspective
    float currentDepth = projCoords.z;

    // Use the geometric normal here so normal maps don't destabilize bias.
    float bias = computeShadowBias(geomNormal, L);

    // If current depth is farther than stored depth, fragment is in shadow
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}

float computeShadowPCF(vec3 geomNormal, vec3 L) {
    vec3 projCoords = getShadowProjCoords();

    // Skip fragments outside the light frustum
    if(projCoords.z > 1.0) return 0.0;
    if(projCoords.x < 0.0 || projCoords.x > 1.0 ||
       projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    float bias = computeShadowBias(geomNormal, L);

    // Size of one texel in the shadow map
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float shadow = 0.0;

    // 3x3 kernel sampling
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}


void main() {
    // Normal selection 
    vec3 geomNormal = normalize(normalWS);
    vec3 N = geomNormal;
    if (useNormalMap) {
        // Sample tangent-space normal from texture
        vec3 normalTS = texture(normal0, texCoord * uvScale).rgb;
        // Unpack from [0,1] to [-1,1]
        normalTS = normalTS * 2.0 - 1.0;
        // Apply normal strength
        normalTS.xy *= normalStrength;
        // DirectX normal map fix
        normalTS.y = -normalTS.y;
        // Re-normalize after strength adjustment
        normalTS = normalize(normalTS);
        // Transform normal from tangent space to world space
        N = normalize(TBN * normalTS);
    }

    // Lighting Vectors
    vec3 L = normalize(-lightDir);
    vec3 V = normalize(camPos - currPos);
    vec3 H = normalize(L + V);  // Halfway vector for Blinn-Phong
        
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


    float shadow = usePCF ? computeShadowPCF(geomNormal, L) : computeShadowHard(geomNormal, L);
    
    // Combine
    vec3 result = baseColor * ambient;
    result += (1.0 - shadow) * diffuse;
    result += (1.0 - shadow) * specular * lightColor.rgb;

    fragColor = vec4(result, 1.0);
    //fragColor = vec4(normalize(N) * 0.5 + 0.5, 1.0);
}
