#version 330 core

in vec3 currPos;       // Receive the current position
in vec3 normalWS;		// Receive world space normal
in vec3 vertexColor;   // Receive color from vertex shader
in vec2 texCoord;      // Receive texture coordinates from vertex shader

out vec4 fragColor;

uniform bool useTextures = true; // Toggle texture usage
uniform sampler2D diffuse0; // texture unit for diffuse
uniform sampler2D specular0; // texture unit for specular
uniform float uvScale = 1.0;

uniform vec4 lightColor; // Gets the color of the light
uniform vec3 lightPos;   // Gets the position of the light
uniform vec3 camPos; // Gets the position of the camera

uniform samplerCube environmentMap; // Environment skybox


void main() {
	// Lighting Vectors
    vec3 N = normalize(normalWS);
    vec3 V = normalize(camPos - currPos);
    vec3 I = normalize(currPos - camPos); // Incident vector

    // Sample textures with fallback
    //vec4 baseColor = useTextures ? texture(diffuse0, texCoord * uvScale) : vec4(vertexColor, 1.0);

    // Frenel Function
    float NdotV = max(dot(N, V), 0.0);
    float F0 = 0.04; // base reflectivity for dielectrics
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0); // Schlick's approximation 

    // Reflection and Refraction    
    float eta = 1.00 / 1.52; // Air to glass refraction index
    vec3 R = reflect(I, N);
    vec3 T = refract(I, N, eta);

    vec3 reflectedColor = texture(environmentMap, R).rgb;
    vec3 refractedColor = texture(environmentMap, T).rgb;

    vec3 result = mix(refractedColor, reflectedColor, fresnel);
    fragColor = vec4(result, 1.0);

}