#include <iostream>
#include <engine/AppSetup.h>
#include <engine/Camera.h>
#include <engine/Texture.h>
#include <engine/Material.h>
#include <engine/Mesh.h>
#include <engine/Model.h>
#include <engine/Shader.h>
#include <engine/MathUtils.h>
#include <engine/Geometry.h>
#include <engine/ShadowMap.h>
#include "ShadowLabUI.h"

// skybox
#include <engine/HDRTexture.h>
#include <engine/Cubemap.h>
#include <engine/HDRConverter.h>
#include <engine/Skybox.h>

// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <chrono>

// -------------------- Establish globals --------------------

const unsigned int width = 1200;
const unsigned int height = 800;
static void beginFrame(TweakableParams& params, ShadowMap& shadowMap, const RuntimeMetrics& metrics) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    buildShadowLabUI(params, shadowMap, metrics);

    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// -------------------- Render Objects --------------------

static void renderMesh(Mesh& mesh, Shader& shader, Camera& camera, TweakableParams& params, glm::mat4 model ) {
    // Ensure correct depth state before drawing 3D geometry
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    // Set shader uniforms
    shader.Activate();
    camera.Matrix(shader, "camMatrix");
    shader.setMat4("model", model);

    // Camera & light
    shader.setVec3("camPos", camera.Position);
    shader.setVec3("lightDir", glm::normalize(-params.position));
    shader.setVec4("lightColor", params.color * params.intensity);
    shader.setFloat("ambient", params.ambient);

    // Toggles
    shader.setBool("useTextures", params.useTextures);
    shader.setBool("useNormalMap", params.useNormalMap);
    shader.setBool("useSignedMSMDepth", params.useSignedMSMDepth);
    shader.setBool("useImprovedMSMBiasTarget", params.useImprovedMSMBiasTarget);
    shader.setInt("shadowMode", static_cast<int>(params.shadowMode));
    shader.setInt("pcfRadius", params.pcfRadius);
    shader.setFloat("msmMomentBias", params.msmMomentBias);
    shader.setFloat("msmReceiverBiasScale", params.msmReceiverBiasScale);
    shader.setFloat("msmOverdarkening", params.msmOverdarkening);
    shader.setFloat("shadowBiasSlope", params.shadowBiasSlope);
    shader.setFloat("shadowBiasMin", params.shadowBiasMin);

    mesh.Draw(shader);
}

void renderLightGizmo(Mesh& mesh, Shader& shader, Camera& camera, TweakableParams& params) {
    shader.Activate();
    camera.Matrix(shader, "camMatrix");

    // If orbiting is enabled, update light position based on time
    if (params.orbitLight) {
        float t = glfwGetTime() * params.orbitSpeed;
        params.position.x = cos(t) * params.orbitRadius;
        params.position.z = sin(t) * params.orbitRadius;
    }

    // make gizmo at light position, with a simple pulsing scale animation
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, params.position);
    float s = 0.1f + 0.02f * sin(glfwGetTime() * 4.0f);
    model = glm::scale(model, glm::vec3(s));

    shader.setMat4("model", model);
    shader.setVec4("lightColor", params.color);

    glDisable(GL_CULL_FACE);
    mesh.Draw(shader);
    glEnable(GL_CULL_FACE);
}

static void renderModel(Model& model, Shader& shader, Camera& camera,
    const TweakableParams& params, float angle) {
    // Set shader uniforms
    shader.Activate();
    camera.Matrix(shader, "camMatrix");

    // Camera & light
    shader.setVec3("camPos", camera.Position);
    shader.setVec3("lightDir", glm::normalize(-params.position));
    shader.setVec4("lightColor", params.color * params.intensity);
    shader.setFloat("ambient", params.ambient);

    // Toggles
    shader.setBool("useTextures", params.useTextures);
    shader.setBool("useNormalMap", params.useNormalMap);
    shader.setBool("useSignedMSMDepth", params.useSignedMSMDepth);
    shader.setBool("useImprovedMSMBiasTarget", params.useImprovedMSMBiasTarget);
    shader.setInt("shadowMode", static_cast<int>(params.shadowMode));
    shader.setInt("pcfRadius", params.pcfRadius);
    shader.setFloat("msmMomentBias", params.msmMomentBias);
    shader.setFloat("msmReceiverBiasScale", params.msmReceiverBiasScale);
    shader.setFloat("msmOverdarkening", params.msmOverdarkening);
    shader.setFloat("shadowBiasSlope", params.shadowBiasSlope);
    shader.setFloat("shadowBiasMin", params.shadowBiasMin);

    model.setRotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model.Draw(shader);
}


// -------------------- Shadow Mapping --------------------

static void renderShadowPass(ShadowMap& shadowMap, const TweakableParams& params,
    Mesh& cube, const glm::mat4& cubeModel,
    Mesh& sphere, const glm::mat4& sphereModel,
    Mesh& pyramid, const glm::mat4& pyramidModel,
    Model& spaceRobot, Model& penguinBot) {
    // Activate the engine-owned shadow shader and push the shared pass uniforms
    shadowMap.bindPassShader();
    Shader& shadowShader = shadowMap.getPassShader();

    glDisable(GL_CULL_FACE);
    shadowMap.Begin();

    shadowShader.setMat4("model", cubeModel);
    cube.Draw(shadowShader);

    shadowShader.setMat4("model", sphereModel);
    sphere.Draw(shadowShader);

    shadowShader.setMat4("model", pyramidModel);
    pyramid.Draw(shadowShader);

    shadowShader.setMat4("model", spaceRobot.getModelMatrix());
    spaceRobot.Draw(shadowShader);

    shadowShader.setMat4("model", penguinBot.getModelMatrix());
    penguinBot.Draw(shadowShader);

    shadowMap.End();
    glEnable(GL_CULL_FACE);
}

static void updateCameraAndInput(GLFWwindow* window, Camera& camera, float dt,
    bool& pWasDown, const glm::vec3& target) {
    bool pDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pDown && !pWasDown) {
        camera.ToggleCinema(target);
    }
    pWasDown = pDown;

    camera.UpdateWithMode(window, dt);
    camera.updateMatrix(0.5f, 100.0f);
}

// -------------------- Main --------------------

int main() {
    std::cout << "Testing" << std::endl;

    // ------------ Initialize the Window ------------

    // create a window
    GLFWwindow* window = initWindow(width, height, "Testing");
    if (!window) return -1;

    // sanity check for smooth camera motion
    glfwSwapInterval(1);

    if (!setupOpenGL()) return -1;

    // Creates camera object
    Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));
	setupCamera(window, camera);

    // Initialize ImGui
    initImGui(window);

    // Load HDR texture for skybox
    HDRTexture hdri("Environment/skybox.hdr");
    Cubemap environment(512);
	HDRConverter converter(512);
	converter.convert(hdri, environment);
	Skybox skybox(environment);

	// ------------ Load Shaders ------------

    std::cout << "Loading shaders..." << std::endl;
    Shader sceneShader("Shaders/scene.vert", "Shaders/scene.frag");
    Shader lightShader("Shaders/light.vert", "Shaders/light.frag");

    // ------------ Shadow Map Logic ------------

    ShadowMap shadowMap(2048, 2048);

    // ------------ Setup Meshes ------------

    sceneShader.Activate();
    std::cout << "Initializing scene meshes..." << std::endl;
    
    auto lightGizmo = Geometry::createSphereMesh();
    auto cube = Geometry::createCubeMesh();
    auto sphere = Geometry::createSphereMesh();
    auto pyramid = Geometry::createPyramidMesh();
    auto plane = Geometry::createPlaneMesh(50.0f, 50.0f, 4.0f);

    auto brickMat = Material::CreateMat(
        "Textures/brick/diffuse.png",
        "Textures/brick/normal.png",
        "Textures/brick/rough.png"
    );
    auto rockMat = Material::CreateMat(
        "Textures/rock/diffuse.png",
        "Textures/rock/normal.png",
        "Textures/rock/rough.png"
    );
    auto woodMat = Material::CreateMat(
        "Textures/wood/diffuse.png",
        "Textures/wood/normal.png",
        "Textures/wood/rough.png"
    );
    auto beachMat = Material::CreateMat(
        "Textures/beach/diffuse.png",
        "Textures/beach/normal.png",
        "Textures/beach/rough.png"
    );
    cube->setMaterial(brickMat);
    sphere->setMaterial(woodMat);
    pyramid->setMaterial(rockMat);
    plane->setMaterial(beachMat);

    glm::mat4 cubeModel(1.0f);
    glm::mat4 sphereModel(1.0f);
    glm::mat4 pyramidModel(1.0f);

    // ------------ Setup Model ------------

    float t0 = (float)glfwGetTime();
    Model spaceRobot("Models/space_robot.glb");
    Model penguinBot("Models/penguin-bot.glb");
    // Model model("Models/teapot/teapot.fbx");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] Models took " << (t1 - t0) << "s\n";

    spaceRobot.setScale(glm::vec3(2.0f));
    penguinBot.setScale(glm::vec3(0.5f));

    spaceRobot.setPosition(glm::vec3(-9.0f, 0.8f, 0.0f));
    penguinBot.setPosition(glm::vec3(9.0f, -0.35f, 0.0f));

    // ------------ Render Loop ------------

    TweakableParams params;
    RuntimeMetrics metrics;
    float prevTime = (float)glfwGetTime();
	bool pWasDown = true;
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    float angle = 0.0f;
    float rotationSpeed = 0.5f; // radians per second
    shadowMap.BindTexture(5);
    sceneShader.setInt("shadowMap", 5);
	std::cout << "Entering render loop..." << std::endl;
    // this loop will run until we close window
    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        float now = (float)glfwGetTime();
        float dt = now - prevTime;
        prevTime = now;
        if (!params.pauseRotation) {
            angle = now * rotationSpeed;
        }

        beginFrame(params, shadowMap, metrics);

        if (params.shadowMode == SceneShadowMode::MSM) {
            shadowMap.setMode(ShadowMode::MSM);
        } else {
            shadowMap.setMode(ShadowMode::Depth);
        }
        shadowMap.resize(params.shadowResolution, params.shadowResolution);
        shadowMap.setUseSignedDepth(params.useSignedMSMDepth);
        shadowMap.setBlurEnabled(params.useMSMBlur);
        shadowMap.setBlurScale(params.blurScale);

        // Build model matrices
        pyramidModel = MathUtils::buildTRS(glm::vec3(-4.5f, 0.45f, 0.0f), glm::vec3(0,1,0), angle, glm::vec3(2.0f));
        sphereModel = MathUtils::buildTRS(glm::vec3(0.0f, 0.55f, 0.0f), glm::vec3(0,1,0), angle, glm::vec3(1.25f));
        cubeModel = MathUtils::buildTRS(glm::vec3(4.5f, 0.4f, 0.0f), glm::vec3(0,1,0), angle, glm::vec3(1.75f));
        glm::mat4 planeModel = MathUtils::buildTRS(
            glm::vec3(0.0f, -1.0f, 0.0f),   // move down slightly
            glm::vec3(0,0,1),
            0.0f,
            glm::vec3(1.0f)
        );

        renderLightGizmo(*lightGizmo, lightShader, camera, params);
        // update shadow light direction from scene light
        glm::vec3 lightDir = glm::normalize(-params.position);

        shadowMap.setDirectionalLight(
            lightDir,
            params.shadowOrthoSize,
            params.shadowNearPlane,
            params.shadowFarPlane
        );

        auto shadowStart = std::chrono::high_resolution_clock::now();
        renderShadowPass(shadowMap, params,
            *cube, cubeModel,
            *sphere, sphereModel,
            *pyramid, pyramidModel,
            spaceRobot, penguinBot
        );
        auto shadowEnd = std::chrono::high_resolution_clock::now();
        metrics.shadowPassMs = std::chrono::duration<float, std::milli>(shadowEnd - shadowStart).count();
        metrics.blurMs = static_cast<float>(shadowMap.getLastBlurMs());

        // restore window viewport
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        updateCameraAndInput(window, camera, dt, pWasDown, target);        

        sceneShader.Activate();
        shadowMap.applyUniforms(sceneShader);
        shadowMap.BindTexture(5);
        
        renderMesh(*cube, sceneShader, camera, params, cubeModel);
        renderMesh(*sphere, sceneShader, camera, params, sphereModel);
        renderMesh(*pyramid, sceneShader, camera, params, pyramidModel);
        renderMesh(*plane, sceneShader, camera, params, planeModel);

        renderModel(spaceRobot, sceneShader, camera, params, angle * 20.0f);
        renderModel(penguinBot, sceneShader, camera, params, angle * 20.0f);

        // Render skybox last
		skybox.Draw(camera);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // unbind the VAO
        glBindVertexArray(0);
        // swap front and back buffers
        glfwSwapBuffers(window);
        // take care of all GLFW events
        glfwPollEvents();

        auto frameEnd = std::chrono::high_resolution_clock::now();
        float frameMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        float fps = frameMs > 0.0f ? 1000.0f / frameMs : 0.0f;
        metrics.averageFrameMs = metrics.averageFrameMs == 0.0f
            ? frameMs
            : metrics.averageFrameMs * 0.9f + frameMs * 0.1f;
        metrics.averageFps = metrics.averageFps == 0.0f
            ? fps
            : metrics.averageFps * 0.9f + fps * 0.1f;
    }

    // ------------ Clean up ------------
    
    sceneShader.Delete();
    lightShader.Delete();

    shutdownImGui();
    shutdownWindow(window);

    return 0;
}
