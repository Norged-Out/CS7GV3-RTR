#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include "Camera.h"
#include "Model.h"
#include "Shader.h"


// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


// -------------------- Establish globals --------------------

const unsigned int width = 1200;
const unsigned int height = 800;

struct LightingParams {
    float intensity = 2.5f;
    glm::vec3 position = glm::vec3(0.0f, 3.0f, 2.0f);
    glm::vec4 color = glm::vec4(1.0f, 0.97f, 0.92f, 1.0f);
    float ambient = 0.25f;

    // Blinn-Phong
    float specularStr = 0.5f;
    float shininess = 32.0f;

    // Toon
    int toonLevels = 3;
    bool enableRim = false;
    float rimStrength = 0.3f;

	// Cook-Torrance
    float metallic = 0.0f;
    float roughness = 0.5f;
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
    camera.Position = glm::vec3(0.0f, 2.0f, 10.0f);   // back a bit, slightly up
    glm::vec3 dir = glm::normalize(target - camera.Position);
    camera.Orientation = dir;
    camera.pitch = glm::degrees(asin(dir.y));
    camera.yaw = glm::degrees(atan2(dir.z, dir.x));
}

void buildGUI(LightingParams& params) {
    ImGui::Begin("Lighting Controls");
    ImGui::Text("Adjust lighting parameters:");
    ImGui::Separator();

    ImGui::SliderFloat("Light Intensity", &params.intensity, 0.5f, 5.0f);
    ImGui::SliderFloat("Ambient", &params.ambient, 0.0f, 1.0f);
    ImGui::ColorEdit3("Light Color", &params.color.r);
    ImGui::DragFloat3("Light Position", &params.position.x, 0.1f);
    ImGui::Separator();

    ImGui::Text("Blinn-Phong (Center):");
    ImGui::SliderFloat("Specular Strength", &params.specularStr, 0.0f, 2.0f);
    ImGui::SliderFloat("Shininess", &params.shininess, 1.0f, 128.0f);
    ImGui::Separator();

    ImGui::Text("Toon Shader (Left):");
    ImGui::SliderInt("Toon Levels", &params.toonLevels, 2, 5);
    ImGui::Checkbox("Enable Rim", &params.enableRim);
    ImGui::SliderFloat("Rim Strength", &params.rimStrength, 0.0f, 1.0f);
    ImGui::Separator();

    ImGui::Text("Cook-Torrance (Right):");
    ImGui::SliderFloat("Metallic", &params.metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &params.roughness, 0.04f, 1.0f);

    ImGui::End();
}

void renderTeapot(Model& teapot, Shader& shader, Camera& camera,
    const LightingParams& params, float angle) {
    shader.Activate();
    camera.Matrix(shader, "camMatrix");

    // Common uniforms
    glm::vec4 finalLightColor = params.color * params.intensity;
    shader.setVec3("camPos", camera.Position);
    shader.setVec4("lightColor", finalLightColor);
    shader.setVec3("lightPos", params.position);
    shader.setFloat("ambient", params.ambient);

    // Shader-specific uniforms (check if uniform exists before setting)
    shader.setFloat("specularStr", params.specularStr);
    shader.setFloat("shininess", params.shininess);
    shader.setInt("toonLevels", params.toonLevels);
    shader.setBool("enableRim", params.enableRim);
    shader.setFloat("rimStrength", params.rimStrength);
    shader.setFloat("metallic", params.metallic);
    shader.setFloat("roughness", params.roughness);

    teapot.setRotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    teapot.Draw(shader);
}

// -------------------- Main --------------------

int main() {
    std::cout << "Assignment 1: Lighting Models Comparison" << std::endl;

    // ------------ Initialize the Window ------------

    // create a window of 800x800 size
    GLFWwindow* window = initWindow(width, height, "Assignment 1: Lighting Models");
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


	// ------------ Load Shaders ------------
    std::cout << "Loading shaders..." << std::endl;

    Shader blinnPhongShader("Shaders/scene.vert", "Shaders/blinnPhong.frag");
    blinnPhongShader.Activate();
	blinnPhongShader.setBool("useTextures", false);
    blinnPhongShader.setInt("diffuse0", 0);
    blinnPhongShader.setInt("specular0", 1);

	Shader toonShader("Shaders/scene.vert", "Shaders/toon.frag");
	toonShader.Activate();
    toonShader.setBool("useTextures", false);
	toonShader.setInt("diffuse0", 0);
	toonShader.setInt("specular0", 1);

	Shader cookTorranceShader("Shaders/scene.vert", "Shaders/cookTorrance.frag");
	cookTorranceShader.Activate();
    cookTorranceShader.setBool("useTextures", false);
	cookTorranceShader.setInt("diffuse0", 0);
	cookTorranceShader.setInt("specular0", 1);

    // ------------ Load Models ------------
    std::cout << "Loading models..." << std::endl;

	// attempt to load teapot model
    float t0 = (float)glfwGetTime();
	Model teapot1("Models/clay-teapot/teapot.fbx");
    Model teapot2("Models/clay-teapot/teapot.fbx");
    Model teapot3("Models/clay-teapot/teapot.fbx");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] teapots took " << (t1 - t0) << "s\n";
    
    // Teapot 1 - Center (Blinn-Phong)
    teapot1.setScale(glm::vec3(0.01f));
    teapot1.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    // Teapot 2 - Left (Toon)
    teapot2.setScale(glm::vec3(0.01f));
    teapot2.setPosition(glm::vec3(-4.0f, 0.0f, 0.0f));

    // Teapot 3 - Right (Cook-Torrance)
    teapot3.setScale(glm::vec3(0.01f));
    teapot3.setPosition(glm::vec3(4.0f, 0.0f, 0.0f));


	// ------------ Lighting Parameters ------------
	LightingParams lightingParams;
	// references for easy access

    // ------------ Render Loop ------------
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
		buildGUI(lightingParams);

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
        // Updates and exports the camera matrix to the Vertex ShadeR
        camera.UpdateWithMode(window, dt);
        camera.updateMatrix(0.5f, 100.0f);

        // Render scene
        renderTeapot(teapot1, blinnPhongShader, camera, lightingParams, angle);
        renderTeapot(teapot2, toonShader, camera, lightingParams, angle);
        renderTeapot(teapot3, cookTorranceShader, camera, lightingParams, angle);
     
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
    blinnPhongShader.Delete();
	toonShader.Delete();
	cookTorranceShader.Delete();
    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}