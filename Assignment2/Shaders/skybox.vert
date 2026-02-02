#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 texDir;

uniform mat4 camMatrix;

void main()
{
    texDir = aPos;

    // Remove translation from camMatrix
    mat4 rotOnly = camMatrix;
    rotOnly[3] = vec4(0, 0, 0, 1);

    vec4 pos = rotOnly * vec4(aPos, 1.0);

    // Force depth to far plane
    gl_Position = pos.xyww;
}
