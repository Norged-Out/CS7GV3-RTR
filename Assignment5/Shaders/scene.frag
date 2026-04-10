#version 330 core

in vec3 currPos;       // Receive the current position
in vec3 normalWS;	   // Receive world space normal
in vec3 vertexColor;   // Receive color from vertex shader
in vec2 texCoord;      // Receive texture coordinates from vertex shader
in mat3 TBN;           // Receive tangent-space basis from vertex shader
in vec4 lightSpacePos; // Receive light-space position from vertex shader

out vec4 fragColor; // Final shaded color output

uniform bool useTextures = false; // Toggle texture usage
uniform bool useNormalMap = false; // Toggle normal mapping
uniform bool useSignedMSMDepth = true; // Toggle the MSM depth convention for comparison
uniform bool useImprovedMSMBiasTarget = true; // Toggle the 2017-style fallback target
uniform int shadowMode = 0; // 0 = hard depth, 1 = PCF depth, 2 = MSM
uniform int pcfRadius = 1; // Radius of the PCF sampling kernel

uniform sampler2D diffuse0; // texture unit for diffuse
uniform sampler2D normal0; // normal map
uniform sampler2D roughness0; // texture unit for roughness
uniform sampler2D shadowMap; // shadow map / shadow texture input

uniform float uvScale = 1.0; // UV tiling multiplier for material textures

uniform vec4 lightColor; // Gets the color of the light
uniform vec3 lightDir;   // Direction the light travels in world space
uniform vec3 camPos; // Gets the position of the camera

uniform float ambient; // Ambient strength
uniform float specularStr = 2.5f; // Specular strength
uniform float roughnessBias = 0.0f; // Bias to adjust roughness
uniform float normalStrength = 1.0f; // Strength of normal mapping
uniform float msmMomentBias = 3e-5f; // Strength of the MSM moment stabilization bias
uniform float msmReceiverBiasScale = 0.5f; // Extra receiver-side MSM bias scale
uniform float msmOverdarkening = 0.0f; // Small extra darkening to suppress faint light leaks
uniform float shadowBiasSlope = 0.01f; // Scale factor for slope-sensitive depth bias
uniform float shadowBiasMin = 0.001f; // Minimum baseline depth bias

vec3 getShadowProjCoords() {
    // Convert clip-space position to normalized device coordinates
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;

    // Convert from [-1,1] to [0,1] coordinates
    projCoords = projCoords * 0.5 + 0.5;
    return projCoords;
}

float computeShadowBias(vec3 geomNormal, vec3 L) {
    float ndotl = max(dot(normalize(geomNormal), normalize(L)), 0.0);
    return max(shadowBiasSlope * (1.0 - ndotl), shadowBiasMin);
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

    // Use the geometric normal here so normal maps don't destabilize bias
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

    // Sample a configurable kernel so UI controls can show softness vs cost
    for(int x = -pcfRadius; x <= pcfRadius; ++x) {
        for(int y = -pcfRadius; y <= pcfRadius; ++y) {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    float kernelWidth = float(pcfRadius * 2 + 1);
    shadow /= kernelWidth * kernelWidth;

    return shadow;
}

vec4 getBiasedMoments(vec2 uv) {
    // Sample the 4 stored moments
    vec4 moments = texture(shadowMap, uv);

    // The old paper-style target is simple, while the 2017 target is more robust.
    vec4 legacySignedBiasTarget = vec4(0.0, 1.0, 0.0, 1.0);
    vec4 legacyUnsignedBiasTarget = vec4(0.5, 0.5, 0.5, 0.5);

    // Use the 2017-style fallback target for signed depth in [-1,1]
    vec4 improvedBiasTarget = vec4(0.0, 0.375, 0.0, 0.375);

    // Pick the bias target based on the active experiment controls
    vec4 biasTarget = useSignedMSMDepth ? legacySignedBiasTarget : legacyUnsignedBiasTarget;
    if (useImprovedMSMBiasTarget && useSignedMSMDepth) {
        biasTarget = improvedBiasTarget;
    }

    // Nudge the sampled moments slightly toward a safer valid moment set.
    return mix(moments, biasTarget, msmMomentBias);
}

vec3 solveMomentSystem(vec4 moments, float zf) {
    // Build small matrix from the 4 moments
    float diag0 = 1.0;
    float lower10 = moments.x / diag0;
    float lower20 = moments.y / diag0;

    // Factor the matrix
    float diag1 = max(moments.y - lower10 * lower10 * diag0, 1e-6);
    float lower21 = (moments.z - lower20 * lower10 * diag0) / diag1;

    // last diagonal term
    float diag2 = max(moments.w - lower20 * lower20 * diag0 - lower21 * lower21 * diag1, 1e-6);

    // Build rhs from the current fragment depth
    vec3 rhs = vec3(1.0, zf, zf * zf);

    // Forward solve
    vec3 y;
    y.x = rhs.x;
    y.y = rhs.y - lower10 * y.x;
    y.z = rhs.z - lower20 * y.x - lower21 * y.y;

    // Diagonal solve
    vec3 z;
    z.x = y.x / diag0;
    z.y = y.y / diag1;
    z.z = y.z / diag2;

    // Back solve to get the quadratic coefficients
    vec3 c;
    c.z = z.z;
    c.y = z.y - lower21 * c.z;
    c.x = z.x - lower10 * c.y - lower20 * c.z;
    return c;
}

float computeShadowMSM(vec3 geomNormal, vec3 L) {
    vec3 projCoords = getShadowProjCoords();

    // Skip fragments outside the light frustum
    if(projCoords.z > 1.0) return 0.0;
    if(projCoords.x < 0.0 || projCoords.x > 1.0 ||
       projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    float bias = computeShadowBias(geomNormal, L) * msmReceiverBiasScale;

    // Build the fragment depth in the same convention used when the moments were written
    float zf = projCoords.z;
    if (useSignedMSMDepth) {
        zf = zf * 2.0 - 1.0;
        zf = clamp(zf - bias * 2.0, -1.0, 1.0);
    } else {
        zf = clamp(zf - bias, 0.0, 1.0);
    }

    // Read filtered moments for this sample
    vec4 moments = getBiasedMoments(projCoords.xy);

    // Solve for the quadratic coefficients
    vec3 c = solveMomentSystem(moments, zf);
    float quadA = c.z;
    float quadB = c.y;
    float quadC = c.x;

    // If the quadratic collapses, treat fragment as lit
    if (abs(quadA) < 1e-6) return 0.0;

    // Solve the quadratic and get the two split depths
    float discriminant = max(quadB * quadB - 4.0 * quadA * quadC, 0.0);
    float sqrtDisc = sqrt(discriminant);
    float inverseTwoA = 0.5 / quadA;

    float firstRoot = (-quadB - sqrtDisc) * inverseTwoA;
    float secondRoot = (-quadB + sqrtDisc) * inverseTwoA;
    if (firstRoot > secondRoot) {
        float temp = firstRoot;
        firstRoot = secondRoot;
        secondRoot = temp;
    }

    // Use the paper's piecewise visibility formulas
    float shadow = 0.0;
    if (zf <= firstRoot) {
        shadow = 0.0;
    } else if (zf <= secondRoot) {
        float denom = (secondRoot - firstRoot) * (zf - firstRoot);
        if (abs(denom) > 1e-6) {
            shadow = (zf * secondRoot - moments.x * (zf + secondRoot) + moments.y) / denom;
        }
    } else {
        float denom = (zf - firstRoot) * (zf - secondRoot);
        if (abs(denom) > 1e-6) {
            shadow = 1.0 - (firstRoot * secondRoot - moments.x * (firstRoot + secondRoot) + moments.y) / denom;
        } else {
            shadow = 1.0;
        }
    }

    // Slightly darken the result to hide faint light leaks, then clamp it back to a sane range.
    shadow = shadow / max(1.0 - msmOverdarkening, 1e-6);
    return clamp(shadow, 0.0, 1.0);
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

    // Shadow-map computation
    float shadow = 0.0;
    if (shadowMode == 2) {
        shadow = computeShadowMSM(geomNormal, L);
    } else if (shadowMode == 1) {
        shadow = computeShadowPCF(geomNormal, L);
    } else {
        shadow = computeShadowHard(geomNormal, L);
    }
    
    // Combine
    vec3 result = baseColor * ambient;
    result += (1.0 - shadow) * diffuse;
    result += (1.0 - shadow) * specular * lightColor.rgb;

    fragColor = vec4(result, 1.0);
    //fragColor = vec4(normalize(N) * 0.5 + 0.5, 1.0);
}
