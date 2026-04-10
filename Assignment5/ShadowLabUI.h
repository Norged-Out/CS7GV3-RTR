#pragma once

#include <array>
#include <string>

#include <glm/glm.hpp>

class ShadowMap;

enum class SceneShadowMode {
    HardDepth = 0,
    PCFDepth,
    MSM
};

struct TweakableParams {
    // Light parameters
    float intensity = 1.0f;
    glm::vec3 position = glm::vec3(0.0f, 3.0f, 3.0f);
    glm::vec4 color = glm::vec4(1.0f, 0.97f, 0.92f, 1.0f);
    float ambient = 0.25f;
    bool orbitLight = false;
    bool pauseRotation = false;
    float orbitRadius = 5.0f;
    float orbitSpeed = 0.5f;

    // Scene/material toggles
    bool useTextures = false;
    bool useNormalMap = false;

    // Shared shadow controls
    SceneShadowMode shadowMode = SceneShadowMode::HardDepth;
    int shadowResolution = 2048;
    float shadowOrthoSize = 12.0f;
    float shadowNearPlane = 1.0f;
    float shadowFarPlane = 40.0f;

    // Baseline depth controls
    float shadowBiasSlope = 0.01f;
    float shadowBiasMin = 0.001f;
    int pcfRadius = 1;

    // MSM controls
    float msmMomentBias = 3e-5f;
    float msmReceiverBiasScale = 0.5f;
    float msmOverdarkening = 0.0f;
    bool useSignedMSMDepth = true;
    bool useImprovedMSMBiasTarget = true;
    bool useMSMBlur = true;
    float blurScale = 1.0f;
};

struct RuntimeMetrics {
    float averageFps = 0.0f;
    float averageFrameMs = 0.0f;
    float shadowPassMs = 0.0f;
    float blurMs = 0.0f;
};

struct MetricSnapshot {
    bool captured = false;
    SceneShadowMode shadowMode = SceneShadowMode::HardDepth;
    int shadowResolution = 0;
    int pcfRadius = 0;
    bool useSignedMSMDepth = false;
    bool useImprovedMSMBiasTarget = false;
    bool useMSMBlur = false;
    float blurScale = 0.0f;
    float shadowMemoryMb = 0.0f;
    float shadowBiasSlope = 0.0f;
    float shadowBiasMin = 0.0f;
    float msmMomentBias = 0.0f;
    float msmReceiverBiasScale = 0.0f;
    float msmOverdarkening = 0.0f;
    RuntimeMetrics metrics;
};

const char* getShadowModeLabel(SceneShadowMode mode);
void applyBaselineBiasPreset(TweakableParams& params, const std::string& presetName);
void applyMSMPreset(TweakableParams& params, const std::string& presetName);
void buildShadowLabUI(TweakableParams& params, ShadowMap& shadowMap, const RuntimeMetrics& metrics,
    std::array<MetricSnapshot, 3>& snapshots);
