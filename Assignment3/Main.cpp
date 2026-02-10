/*
* Author: Priyansh Nayak
* Project: Normal Mapping
* Course: CS7GV3: Real-Time Rendering
*/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <engine/Camera.h>
#include <engine/Texture.h>
#include <engine/Mesh.h>
#include <engine/Shader.h>
#include <engine/MathUtils.h>
#include <engine/Geometry.h>

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

// -------------------- Establish globals --------------------

const unsigned int width = 1200;
const unsigned int height = 800;

struct TweakableParams {
    // Light parameters
    float intensity = 1.0f;
    glm::vec3 position = glm::vec3(0.0f, 3.0f, 3.0f);
    glm::vec4 color = glm::vec4(1.0f, 0.97f, 0.92f, 1.0f);
    float ambient = 0.25f;
    bool orbitLight = false;
    float orbitRadius = 5.0f;
    float orbitSpeed = 0.5f; // radians/sec

    // Specular
    float specularStrength = 1.0f;
    float roughnessBias = 0.0f;

    // Rendering toggles
    bool useTextures = false;

    // Normal mapping
    bool useNormalMap = false;
    float normalStrength = 1.0f;
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

// -------------------- Initialize GLAD --------------------

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

// -------------------- Initialize Camera --------------------

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
    camera.Position = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 dir = glm::normalize(target - camera.Position);
    camera.Orientation = dir;
    camera.pitch = glm::degrees(asin(dir.y));
    camera.yaw = glm::degrees(atan2(dir.z, dir.x));
}

// -------------------- GUI Setup --------------------

static void buildGUI(TweakableParams& params) {
    ImGui::Begin("Rotations Controls");
    ImGui::SliderFloat("Light Intensity", &params.intensity, 0.5f, 5.0f);
    ImGui::SliderFloat("Ambient", &params.ambient, 0.0f, 1.0f);
    ImGui::ColorEdit3("Light Color", &params.color.r);
    ImGui::DragFloat3("Light Position", &params.position.x, 0.1f);

    ImGui::Separator();
    ImGui::Checkbox("Orbit Light", &params.orbitLight);
    ImGui::SliderFloat("Orbit Radius", &params.orbitRadius, 1.0f, 10.0f);
    ImGui::SliderFloat("Orbit Speed", &params.orbitSpeed, 0.1f, 1.0f);

    ImGui::Separator();
    ImGui::SliderFloat("Specular Strength", &params.specularStrength, 0.0f, 5.0f);
    ImGui::SliderFloat("Roughness Bias", &params.roughnessBias, -1.0f, 1.0f);

    ImGui::Separator();
    ImGui::Checkbox("Use Textures", &params.useTextures);
    ImGui::Checkbox("Use Normal Map", &params.useNormalMap);
    ImGui::SliderFloat("Normal Strength", &params.normalStrength, 0.0f, 2.0f);

    ImGui::End();
}

// -------------------- Render Sphere --------------------

void renderMesh(Mesh& mesh, Shader& shader, Camera& camera, TweakableParams& params, glm::mat4 model ) {
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
    shader.setVec3("lightPos", params.position);
    shader.setVec4("lightColor", params.color * params.intensity);
    shader.setFloat("ambient", params.ambient);

    // Material
    shader.setFloat("specularStr", params.specularStrength);
    shader.setFloat("roughnessBias", params.roughnessBias);
    shader.setFloat("normalStrength", params.normalStrength);

    // Toggles
    shader.setBool("useTextures", params.useTextures);
    shader.setBool("useNormalMap", params.useNormalMap);

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

// -------------------- Main --------------------

int main() {
    std::cout << "Testing" << std::endl;

    // ------------ Initialize the Window ------------

    // create a window of 800x800 size
    GLFWwindow* window = initWindow(width, height, "Testing");
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
    Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");

    // ------------ Setup Spheres ------------

    sceneShader.Activate();
    sceneShader.setInt("diffuse0", 0);
    sceneShader.setInt("normal0", 2);
    sceneShader.setInt("roughness0", 3);
    //sceneShader.setInt("ao0", 5);

    std::cout << "Initializing sphere meshes..." << std::endl;
    
    std::unique_ptr<Mesh> lightGizmo = Geometry::createTexturedSphere(false);
    std::unique_ptr<Mesh> sphere1 = Geometry::createTexturedSphere(true, "Textures/brick");
    std::unique_ptr<Mesh> sphere2 = Geometry::createTexturedSphere(true, "Textures/rock");
    std::unique_ptr<Mesh> sphere3 = Geometry::createTexturedSphere(true, "Textures/wood");

    float spacing = 2.2f;
    glm::mat4 m1(1.0f);
    glm::mat4 m2(1.0f);
    glm::mat4 m3(1.0f);

    // ------------ Render Loop ------------
    TweakableParams params;
    float prevTime = (float)glfwGetTime();
	bool pWasDown = true;
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    float angle = 0.0f;
    float rotationSpeed = 0.5f; // radians per second
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

        // Build model matrices for spheres
        m1 = MathUtils::buildTRS(glm::vec3(-spacing, 0.0f, 0.0f), glm::vec3(0,1,0), angle, glm::vec3(1.0f));
        m2 = MathUtils::buildTRS(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0,1,0), angle, glm::vec3(1.0f));
        m3 = MathUtils::buildTRS(glm::vec3(spacing, 0.0f, 0.0f), glm::vec3(0,1,0), angle, glm::vec3(1.0f));

        renderLightGizmo(*lightGizmo, lightShader, camera, params);
        renderMesh(*sphere1, sceneShader, camera, params, m1);
        renderMesh(*sphere2, sceneShader, camera, params, m2);
        renderMesh(*sphere3, sceneShader, camera, params, m3);

        // Render skybox last
        skyboxShader.Activate();
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
    lightShader.Delete();
    skyboxShader.Delete();
    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}