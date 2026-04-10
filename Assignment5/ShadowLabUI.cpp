#include "ShadowLabUI.h"

#include <fstream>
#include <iomanip>
#include <imgui.h>
#include <engine/ShadowMap.h>

// Copy the current settings plus the smoothed metrics into one comparison slot.
static void captureSnapshot(MetricSnapshot& snapshot, const TweakableParams& params,
    const RuntimeMetrics& metrics, const ShadowMap& shadowMap) {
    snapshot.captured = true;
    snapshot.shadowMode = params.shadowMode;
    snapshot.shadowResolution = params.shadowResolution;
    snapshot.pcfRadius = params.pcfRadius;
    snapshot.useSignedMSMDepth = params.useSignedMSMDepth;
    snapshot.useImprovedMSMBiasTarget = params.useImprovedMSMBiasTarget;
    snapshot.useMSMBlur = params.useMSMBlur;
    snapshot.blurScale = params.blurScale;
    snapshot.shadowMemoryMb = static_cast<float>(shadowMap.getApproxMemoryBytes() / (1024.0 * 1024.0));
    snapshot.shadowBiasSlope = params.shadowBiasSlope;
    snapshot.shadowBiasMin = params.shadowBiasMin;
    snapshot.msmMomentBias = params.msmMomentBias;
    snapshot.msmReceiverBiasScale = params.msmReceiverBiasScale;
    snapshot.msmOverdarkening = params.msmOverdarkening;
    snapshot.metrics = metrics;
}

// Dump the captured slots into a simple CSV file for later report analysis.
static bool exportSnapshotsToCsv(const std::array<MetricSnapshot, 3>& snapshots) {
    std::ofstream file("shadow_metrics.csv", std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << "slot,mode,resolution,avg_fps,avg_frame_ms,shadow_pass_ms,blur_pass_ms,shadow_memory_mb,pcf_radius,"
            "signed_depth,improved_bias_target,blur_enabled,blur_scale,bias_slope,bias_min,moment_bias,"
            "receiver_bias_scale,overdarkening\n";

    file << std::fixed << std::setprecision(4);

    for (size_t i = 0; i < snapshots.size(); ++i) {
        const MetricSnapshot& shot = snapshots[i];
        if (!shot.captured) {
            continue;
        }

        file
            << (i + 1) << ','
            << getShadowModeLabel(shot.shadowMode) << ','
            << shot.shadowResolution << ','
            << shot.metrics.averageFps << ','
            << shot.metrics.averageFrameMs << ','
            << shot.metrics.shadowPassMs << ','
            << shot.metrics.blurMs << ','
            << shot.shadowMemoryMb << ','
            << shot.pcfRadius << ','
            << (shot.useSignedMSMDepth ? 1 : 0) << ','
            << (shot.useImprovedMSMBiasTarget ? 1 : 0) << ','
            << (shot.useMSMBlur ? 1 : 0) << ','
            << shot.blurScale << ','
            << shot.shadowBiasSlope << ','
            << shot.shadowBiasMin << ','
            << shot.msmMomentBias << ','
            << shot.msmReceiverBiasScale << ','
            << shot.msmOverdarkening << '\n';
    }

    return true;
}

const char* getShadowModeLabel(SceneShadowMode mode) {
    switch (mode) {
    case SceneShadowMode::HardDepth: return "Hard";
    case SceneShadowMode::PCFDepth:  return "PCF";
    case SceneShadowMode::MSM:       return "MSM";
    default:                         return "Unknown";
    }
}

void applyBaselineBiasPreset(TweakableParams& params, const std::string& presetName) {
    if (presetName == "None") {
        params.shadowBiasSlope = 0.0f;
        params.shadowBiasMin = 0.0f;
    } else if (presetName == "Legacy") {
        params.shadowBiasSlope = 0.004f;
        params.shadowBiasMin = 0.0002f;
    } else {
        params.shadowBiasSlope = 0.01f;
        params.shadowBiasMin = 0.001f;
    }
}

void applyMSMPreset(TweakableParams& params, const std::string& presetName) {
    if (presetName == "Raw") {
        params.useSignedMSMDepth = false;
        params.useImprovedMSMBiasTarget = false;
        params.useMSMBlur = false;
        params.msmMomentBias = 0.0f;
        params.msmReceiverBiasScale = 0.0f;
        params.msmOverdarkening = 0.0f;
        params.blurScale = 1.0f;
    } else if (presetName == "Legacy") {
        params.useSignedMSMDepth = false;
        params.useImprovedMSMBiasTarget = false;
        params.useMSMBlur = false;
        params.msmMomentBias = 3e-5f;
        params.msmReceiverBiasScale = 0.0f;
        params.msmOverdarkening = 0.0f;
        params.blurScale = 1.0f;
    } else {
        params.useSignedMSMDepth = true;
        params.useImprovedMSMBiasTarget = true;
        params.useMSMBlur = true;
        params.msmMomentBias = 3e-5f;
        params.msmReceiverBiasScale = 0.5f;
        params.msmOverdarkening = 0.0f;
        params.blurScale = 1.0f;
    }
}

void buildShadowLabUI(TweakableParams& params, ShadowMap& shadowMap, const RuntimeMetrics& metrics,
    std::array<MetricSnapshot, 3>& snapshots) {
    ImGui::Begin("Rotations Controls");

    // General scene and lighting controls stay visible regardless of shadow mode.
    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Light Intensity", &params.intensity, 0.5f, 5.0f);
        ImGui::SliderFloat("Ambient", &params.ambient, 0.0f, 1.0f);
        ImGui::ColorEdit3("Light Color", &params.color.r);
        ImGui::DragFloat3("Light Position", &params.position.x, 0.1f);
    }

    if (ImGui::CollapsingHeader("Scene Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Orbit Light", &params.orbitLight);
        ImGui::Checkbox("Pause Rotation", &params.pauseRotation);
        ImGui::SliderFloat("Orbit Radius", &params.orbitRadius, 1.0f, 10.0f);
        ImGui::SliderFloat("Orbit Speed", &params.orbitSpeed, 0.1f, 1.0f);
    }

    if (ImGui::CollapsingHeader("Material Toggles", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Use Textures", &params.useTextures);
        ImGui::Checkbox("Use Normal Map", &params.useNormalMap);
    }

    if (ImGui::CollapsingHeader("Shadow Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* shadowModeLabels[] = { "Hard Depth", "PCF Depth", "MSM" };
        int shadowModeIndex = static_cast<int>(params.shadowMode);
        ImGui::Combo("Shadow Mode", &shadowModeIndex, shadowModeLabels, IM_ARRAYSIZE(shadowModeLabels));
        params.shadowMode = static_cast<SceneShadowMode>(shadowModeIndex);

        const std::array<int, 4> shadowResolutions = { 1024, 2048, 3072, 4096 };
        int resolutionIndex = 1;
        for (size_t i = 0; i < shadowResolutions.size(); ++i) {
            if (shadowResolutions[i] == params.shadowResolution) {
                resolutionIndex = static_cast<int>(i);
                break;
            }
        }

        const char* resolutionLabels[] = { "1024", "2048", "3072", "4096" };
        ImGui::Combo("Shadow Resolution", &resolutionIndex, resolutionLabels, IM_ARRAYSIZE(resolutionLabels));
        params.shadowResolution = shadowResolutions[resolutionIndex];

        ImGui::SliderFloat("Ortho Size", &params.shadowOrthoSize, 4.0f, 20.0f);
        ImGui::SliderFloat("Near Plane", &params.shadowNearPlane, 0.1f, 10.0f);
        ImGui::SliderFloat("Far Plane", &params.shadowFarPlane, 10.0f, 80.0f);
    }

    // Baseline-only controls only matter for hard shadows and PCF.
    if (params.shadowMode != SceneShadowMode::MSM &&
        ImGui::CollapsingHeader("Baseline Bias", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("No Baseline Bias")) applyBaselineBiasPreset(params, "None");
        ImGui::SameLine();
        if (ImGui::Button("Legacy Baseline")) applyBaselineBiasPreset(params, "Legacy");
        ImGui::SameLine();
        if (ImGui::Button("Current Baseline")) applyBaselineBiasPreset(params, "Current");

        ImGui::SliderFloat("Bias Slope", &params.shadowBiasSlope, 0.0f, 0.03f, "%.4f");
        ImGui::SliderFloat("Bias Min", &params.shadowBiasMin, 0.0f, 0.005f, "%.5f");
    }

    // PCF controls only show up when PCF is active.
    if (params.shadowMode == SceneShadowMode::PCFDepth &&
        ImGui::CollapsingHeader("PCF Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderInt("PCF Radius", &params.pcfRadius, 1, 4);
        int sampleCount = (params.pcfRadius * 2 + 1) * (params.pcfRadius * 2 + 1);
        ImGui::Text("Current PCF Samples: %d", sampleCount);
    }

    // MSM controls expose both old and current settings for report comparisons.
    if (params.shadowMode == SceneShadowMode::MSM &&
        ImGui::CollapsingHeader("MSM Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("Raw MSM")) applyMSMPreset(params, "Raw");
        ImGui::SameLine();
        if (ImGui::Button("Legacy MSM")) applyMSMPreset(params, "Legacy");
        ImGui::SameLine();
        if (ImGui::Button("Current MSM")) applyMSMPreset(params, "Current");

        ImGui::Checkbox("Use Signed Depth", &params.useSignedMSMDepth);
        ImGui::Checkbox("Use Improved Bias Target", &params.useImprovedMSMBiasTarget);
        ImGui::Checkbox("Use Gaussian Blur", &params.useMSMBlur);
        ImGui::SliderFloat("Moment Bias", &params.msmMomentBias, 0.0f, 0.0002f, "%.7f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("Receiver Bias Scale", &params.msmReceiverBiasScale, 0.0f, 2.0f);
        ImGui::SliderFloat("Overdarkening", &params.msmOverdarkening, 0.0f, 0.1f, "%.3f");
        ImGui::SliderFloat("Blur Scale", &params.blurScale, 0.5f, 3.0f, "%.2f");
    }

    // Keep only the metrics that help compare quality/cost across modes.
    if (ImGui::CollapsingHeader("Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Average FPS: %.1f", metrics.averageFps);
        ImGui::Text("Average Frame Time: %.3f ms", metrics.averageFrameMs);
        ImGui::Text("Shadow Pass: %.3f ms", metrics.shadowPassMs);
        ImGui::Text("Blur Pass: %.3f ms", metrics.blurMs);
        ImGui::Text("Shadow Resolution: %u x %u", shadowMap.getWidth(), shadowMap.getHeight());
        ImGui::Text("Shadow Memory: %.2f MB", shadowMap.getApproxMemoryBytes() / (1024.0f * 1024.0f));
    }

    // Capture slots make it easier to record stable values for the report.
    if (ImGui::CollapsingHeader("Capture Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("Freeze the scene, wait a moment for the averages to settle, then capture a slot.");
        if (ImGui::Button("Export Captures to CSV")) {
            exportSnapshotsToCsv(snapshots);
        }
        ImGui::Text("Output file: shadow_metrics.csv");

        for (int i = 0; i < 3; ++i) {
            std::string captureLabel = "Capture Slot " + std::to_string(i + 1);
            if (ImGui::Button(captureLabel.c_str())) {
                captureSnapshot(snapshots[i], params, metrics, shadowMap);
            }
            ImGui::SameLine();
            std::string clearLabel = "Clear##" + std::to_string(i + 1);
            if (ImGui::Button(clearLabel.c_str())) {
                snapshots[i] = MetricSnapshot{};
            }

            if (snapshots[i].captured) {
                const MetricSnapshot& shot = snapshots[i];
                ImGui::Text("Slot %d: %s | %d | Avg FPS %.1f | Avg ms %.3f",
                    i + 1,
                    getShadowModeLabel(shot.shadowMode),
                    shot.shadowResolution,
                    shot.metrics.averageFps,
                    shot.metrics.averageFrameMs
                );
                ImGui::Text("  Shadow %.3f ms | Blur %.3f ms | Memory %.2f MB",
                    shot.metrics.shadowPassMs,
                    shot.metrics.blurMs,
                    shot.shadowMemoryMb
                );
                if (shot.shadowMode == SceneShadowMode::PCFDepth) {
                    ImGui::Text("  PCF radius: %d", shot.pcfRadius);
                }
                if (shot.shadowMode == SceneShadowMode::MSM) {
                    ImGui::Text("  Signed: %s | Bias target: %s | Blur: %s (%.2f)",
                        shot.useSignedMSMDepth ? "On" : "Off",
                        shot.useImprovedMSMBiasTarget ? "Improved" : "Legacy",
                        shot.useMSMBlur ? "On" : "Off",
                        shot.blurScale
                    );
                    ImGui::Text("  Moment %.7f | Receiver %.3f | Overdarkening %.3f",
                        shot.msmMomentBias,
                        shot.msmReceiverBiasScale,
                        shot.msmOverdarkening
                    );
                } else {
                    ImGui::Text("  Bias slope %.4f | Bias min %.5f",
                        shot.shadowBiasSlope,
                        shot.shadowBiasMin
                    );
                }
            } else {
                ImGui::Text("Slot %d: empty", i + 1);
            }
        }
    }

    // The debug preview always shows the active shadow resource.
    ImGui::Separator();
    ImGui::Text("Shadow Map Debug");

    ImGui::Image(
        (ImTextureID)(uintptr_t)shadowMap.getDebugTexture(),
        ImVec2(256,256),
        ImVec2(0,1),
        ImVec2(1,0)
    );

    ImGui::End();
}
