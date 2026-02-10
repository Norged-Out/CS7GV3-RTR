/*
* Author: Priyansh Nayak
* Project: Transmittance Effects
* Course: CS7GV3: Real-Time Rendering
*/

#include <iostream>
#include <engine/AppSetup.h>
#include <engine/Camera.h>
#include <engine/Model.h>
#include <engine/Shader.h>

// enviroment
#include <engine/HDRTexture.h>
#include <engine/Cubemap.h>
#include <engine/HDRConverter.h>
#include <engine/Skybox.h>

// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// -------------------- Establish globals --------------------

const unsigned int width = 1200;
const unsigned int height = 800;

struct TweakableParams {
    float baseIOR = 1.52f;        // Glass default
    float fresnelPower = 5.0f;    // Schlick exponent
    float dispersion = 0.01f;     // Small wavelength offset

    // HDR exposure
    float exposure = 1.0f;        
    float skyboxExposure = 0.7f;

    bool enableReflection = true;
    bool enableRefraction = true;
};

static void buildGUI(TweakableParams& params) {
    ImGui::Begin("Transmission Controls");

    ImGui::Text("Optical Properties");
    ImGui::SliderFloat("Base IOR", &params.baseIOR, 1.0f, 2.0f);
    ImGui::SliderFloat("Fresnel Power", &params.fresnelPower, 1.0f, 10.0f);
    ImGui::SliderFloat("Dispersion Strength", &params.dispersion, 0.0f, 0.05f);

    ImGui::Separator();

    ImGui::Text("Rendering");
    ImGui::SliderFloat("Exposure", &params.exposure, 0.1f, 5.0f);
    ImGui::SliderFloat("Skybox Exposure", &params.skyboxExposure, 0.1f, 3.0f);
    ImGui::Checkbox("Enable Reflection", &params.enableReflection);
    ImGui::Checkbox("Enable Refraction", &params.enableRefraction);

    ImGui::End();
}

static void renderModel(Model& model, Shader& shader, Camera& camera,
    const TweakableParams& params, float angle) {
    shader.Activate();
    camera.Matrix(shader, "camMatrix");

    // Controllable uniforms
    shader.setVec3("camPos", camera.Position);
    shader.setFloat("baseIOR", params.baseIOR);
    shader.setFloat("fresnelPower", params.fresnelPower);
    shader.setFloat("dispersion", params.dispersion);
    shader.setFloat("exposure", params.exposure);
    shader.setBool("enableReflection", params.enableReflection);
    shader.setBool("enableRefraction", params.enableRefraction);

    model.setRotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model.Draw(shader);
}

// -------------------- Main --------------------

int main() {
    std::cout << "Assignment 2: Transmittance Effects" << std::endl;

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

	// Load HDR texture for environment mapping
    HDRTexture hdri("Environment/lakeside_sunrise.hdr");
    Cubemap environment(512);
	HDRConverter converter(512);
	converter.convert(hdri, environment);
	Skybox skybox(environment);

	// ------------ Load Shaders ------------
    std::cout << "Loading shaders..." << std::endl;

    Shader sceneShader("Shaders/scene.vert", "Shaders/scene.frag");
    sceneShader.Activate();
	environment.Bind(5);
	sceneShader.setInt("environmentMap", 5);

	Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");

    // ------------ Load Models ------------
    std::cout << "Loading models..." << std::endl;

	// attempt to load model
    float t0 = (float)glfwGetTime();
    Model model1("Models/robot-2020/robo.fbx");
    Model model2("Models/space_robot.glb");
    Model model3("Models/penguin-bot.glb");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] Model took " << (t1 - t0) << "s\n";

	
	model1.setPosition(glm::vec3(-5.0f, -2.5f, -2.0f));
    model1.setScale(glm::vec3(0.01f));

	model2.setPosition(glm::vec3(5.0f, 0.0f, -1.0f));
    model2.setScale(glm::vec3(2.5f));

    model3.setPosition(glm::vec3(0.0f, -2.5f, 0.0f));
    model3.setScale(glm::vec3(0.5f));

    // ------------ Render Loop ------------
    TweakableParams params;
    float prevTime = (float)glfwGetTime();
	bool pWasDown = true;
    float rotationSpeed = 20.0f;
	float angle = 0.0f;
    glm::vec3 target(0.0f, 0.0f, 0.0f);
	std::cout << "Entering render loop..." << std::endl;
    // this loop will run until we close window
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - prevTime;
        prevTime = now;
        angle = now * rotationSpeed;

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        buildGUI(params);

        // clear the screen and specify background color
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        // clean back buffer and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Handle camera inputs
        bool pDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pDown && !pWasDown) {
            camera.ToggleCinema(target);
        }
        pWasDown = pDown;
        // Updates and exports the camera matrix to the Vertex Shader
        camera.UpdateWithMode(window, dt);
        camera.updateMatrix(0.5f, 100.0f);
        
        // Render scene
        renderModel(model1, sceneShader, camera, params, angle);
		renderModel(model2, sceneShader, camera, params, angle);
        renderModel(model3, sceneShader, camera, params, angle);

		// Render skybox last
        skyboxShader.Activate();
        skyboxShader.setFloat("skyboxExposure", params.skyboxExposure);
		skybox.Draw(camera, skyboxShader);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // unbind the VAO
        glBindVertexArray(0);
        // swap front and back buffers
        glfwSwapBuffers(window);
        // take care of all GLFW events
        glfwPollEvents();

    }

    // ------------ Clean up ------------
    
    // delete shader program
    sceneShader.Delete();
    skyboxShader.Delete();

    shutdownImGui();
    shutdownWindow(window);


    return 0;

}