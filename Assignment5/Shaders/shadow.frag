#version 330 core

in float shadowDepth; // Receive shadow-space depth from vertex shader

layout (location = 0) out vec4 momentOut; // Output RGBA moments for MSM mode

uniform int shadowMode = 0; // 0 = baseline depth path, 1 = MSM moment-writing path

void main()
{
    // Baseline check
    if (shadowMode == 0) return;

    // MSM mode: store the first four moments of shadow-space depth.
    float z = clamp(shadowDepth, 0.0, 1.0);
    float z2 = z * z;
    momentOut = vec4(z, z2, z2 * z, z2 * z2);
}
