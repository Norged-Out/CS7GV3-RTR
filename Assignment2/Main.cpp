#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include "Camera.h"
#include "Model.h"
#include "Shader.h"

// enviroment
#include "Environment/HDRTexture.h"
#include "Environment/Cubemap.h"
#include "Environment/HDRConverter.h"
#include "Environment/Skybox.h"

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
    float skyboxExposure = 0.8f;

    bool enableReflection = true;
    bool enableRefraction = true;
};

// -------------------- Initialize GLFW --------------------

static GLFWwindow* initWindow(int width, int height, const char* title) {

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return nullptr;
    }

    // tell GLFW to use the core Version 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
    // error check
    if (!window) {
        std::cerr << "Failed to create window!" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    // introduce window to current context
    glfwMakeContextCurrent(window);
    return window;

}

// function for resizing window
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Make sure the viewport matches the new window dimensions
    glViewport(0, 0, width, height);
    // Update camera as well
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (cam) cam->setSize(width, height);
}

static void setupOpenGL() {
    // use GLAD to configure OpenGL
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return;
    }
    // specify window dimensions
    glViewport(0, 0, width, height);
    // Enable depth and backface culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
}

static void setupCamera(GLFWwindow* window, Camera& camera) {
    // attach camera pointer to window
    glfwSetWindowUserPointer(window, &camera);
    // register callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // set scroll callback
    glfwSetScrollCallback(window, [](GLFWwindow* win, double xoff, double yoff) {
        // forward to camera
        Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
        if (cam) cam->OnScroll(yoff);
        });
    // Point camera at scene center
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    camera.Position = glm::vec3(0.0f, 0.0f, 10.0f);   // back a bit, slightly down
    glm::vec3 dir = glm::normalize(target - camera.Position);
    camera.Orientation = dir;
    camera.pitch = glm::degrees(asin(dir.y));
    camera.yaw = glm::degrees(atan2(dir.z, dir.x));
}

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
    std::cout << "Assignment 2: Transmitance Effects" << std::endl;

    // ------------ Initialize the Window ------------

    // create a window of 800x800 size
    GLFWwindow* window = initWindow(width, height, "Assignment 2: Transmitance Effects");
    if (!window) return -1;

    // sanity check for smooth camera motion
    glfwSwapInterval(1);

    // use GLAD to configure OpenGL
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }
    setupOpenGL();

    // Creates camera object
    Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));
	setupCamera(window, camera);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

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
    Model model2("Models/steampunk_robot.glb");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] Model took " << (t1 - t0) << "s\n";

	
	model1.setPosition(glm::vec3(-4.0f, 0.0f, 0.0f));
    model1.setScale(glm::vec3(0.01f));

	model2.setPosition(glm::vec3(4.0f, 0.0f, 0.0f));
	model2.setRotation(180.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    model2.setScale(glm::vec3(5.0f));

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
    
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
	// delete shader program
    sceneShader.Delete();
    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}