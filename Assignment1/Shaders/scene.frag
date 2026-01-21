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


vec4 directLight() {

	// Lighting Vectors
	vec3 N = normalize(normalWS);			// normal direction
    vec3 L = normalize(lightPos);			// light direction 
    vec3 V = normalize(camPos - currPos);   // view direction

	// lighting terms
	float ambient = 0.23f; // constant fill light
	float diffuse = max(dot(N, L), 0.0f); // how much the surface faces the light

	// specular lighting
	float specularStr = 0.40f; // modify for strength
	vec3 viewDir = normalize(camPos - currPos);
	vec3 R = reflect(-L, N); // reflection direction
	float spec = pow(max(dot(V, R), 0.0f), 8.0); // shininess value can be changed
	float specular = specularStr * spec;
	
	return (texture(diffuse0, texCoord) * (diffuse + ambient)
			+ texture(specular0, texCoord).r * specular)
			* lightColor;
}


void main() {
	fragColor = directLight();
}
