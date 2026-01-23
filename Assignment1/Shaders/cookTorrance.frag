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
// Calculate how much light reflects based on viewing angle
float FresnelReflection(float VdotH, float F0) {
    float fresnel = pow(1.0 - VdotH, 5.0); // Quintic falloff
    return F0 + (1.0 - F0) * fresnel;
}

// Geometry Function (Smith's method)
// Models self-shadowing of microfacets
float GeometricShadow(float NdotV, float NdotL, float roughness) {
    float k = roughness * roughness / 2.0; // remap roughness for direct lighting
    float g1 = NdotV / (NdotV * (1.0 - k) + k); // shadowing from view
    float g2 = NdotL / (NdotL * (1.0 - k) + k); // shadowing from light
    return g1 * g2; // combined shadowing
}

void main() {
    // Lighting Vectors
    vec3 N = normalize(normalWS);
    vec3 L = normalize(lightPos - currPos);
    vec3 V = normalize(camPos - currPos);
    vec3 H = normalize(L + V);  // Halfway vector

    // Dot Products that are reused for D, F, G
    float NdotL = max(dot(N, L), 0.0); // how much surface faces light
    float NdotV = max(dot(N, V), 0.0); // how much surface faces camera
    float NdotH = max(dot(N, H), 0.0); // specular alignment
    float VdotH = max(dot(V, H), 0.0); // fresnel calculation

    // Attenuation
    float distance = length(lightPos - currPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);

    // Sample textures
    vec3 albedo = texture(diffuse0, texCoord * uvScale).rgb;
    float roughnessMap = texture(specular0, texCoord * uvScale).r;
    float finalRoughness = roughness * roughnessMap; // combine uniform and texture
    finalRoughness = clamp(finalRoughness, 0.04, 1.0); // avoid 0 roughness

    // Calculate Base Reflectivity F0 based on metalness
    float F0 = mix(0.04, 1.0, metallic); // non-metals reflect ~4%, metals reflect albedo

    // Cook-Torrance BRDF
    float D = GGXDistribution(NdotH, finalRoughness); // no. of microfacets 
    float F = FresnelReflection(VdotH, F0); // reflectivity at angle
    float G = GeometricShadow(NdotV, NdotL, finalRoughness); // shadowing/masking
    float specular = (D * F * G) / max(4.0 * NdotV * NdotL, 0.001);

    // Contribution factors
    float kS = F; // specular reflection
    float kD = (1.0 - kS) * (1.0 - metallic); // diffuse scattering

    // Diffuse and Ambience terms
    vec3 diffuse = (albedo / PI) * kD; // Lambertian diffuse
    vec3 ambientTerm = ambient * albedo; // Ambient term

    // Combine
    vec3 result = ambientTerm + ((diffuse + specular) * lightColor.rgb * NdotL * attenuation);

    fragColor = vec4(result, 1.0);
}
