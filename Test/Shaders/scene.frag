#version 330 core

in vec3 currPos;       // Receive the current position
in vec3 normalWS;		// Receive world space normal
in vec3 vertexColor;   // Receive color from vertex shader
in vec2 texCoord;      // Receive texture coordinates from vertex shader

out vec4 fragColor;

uniform vec3 camPos; // Gets the position of the camera
uniform samplerCube environmentMap; // Environment skybox

// Tweakables
uniform float baseIOR;
uniform float fresnelPower;
uniform float dispersion;
uniform float exposure;

uniform bool enableReflection;
uniform bool enableRefraction;


void main() {
	// Lighting Vectors
    vec3 N = normalize(normalWS);
    vec3 V = normalize(camPos - currPos);
    vec3 I = normalize(currPos - camPos); // Incident ray

    // Frenel Function (Schlick's approximation)
    float NdotV = max(dot(N, V), 0.0);
    float F0 = 0.04; // dielectric base reflectivity
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, fresnelPower);

    // Reflection    
    vec3 reflectedColor = vec3(0.0);
    if (enableReflection) {
        vec3 R = reflect(I, N);
        reflectedColor = texture(environmentMap, R).rgb;
    }
    // Chromatic dispersion (Refraction)
    vec3 refractedColor = vec3(0.0);
    if (enableRefraction) {
        // setup refraction index
        float etaR = 1.0 / (baseIOR - dispersion);
        float etaG = 1.0 / baseIOR;
        float etaB = 1.0 / (baseIOR + dispersion);

        // Compute refracted ray directions
        vec3 TR = refract(I, N, etaR);
        vec3 TG = refract(I, N, etaG);
        vec3 TB = refract(I, N, etaB);

        // Refraction
        refractedColor.r = texture(environmentMap, TR).r;
        refractedColor.g = texture(environmentMap, TG).g;
        refractedColor.b = texture(environmentMap, TB).b;
    }

    // Fresnel blend
    vec3 color = mix(refractedColor, reflectedColor, fresnel);

    // Simple exposure control (HDR toning)
    color *= exposure;

    fragColor = vec4(color, 1.0);
}