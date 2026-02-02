#version 330 core

in vec3 texDir;
out vec4 fragColor;

uniform samplerCube environmentMap;

void main() {
    fragColor = texture(environmentMap, texDir);
}
