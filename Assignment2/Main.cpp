#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include "Camera.h"
#include "Model.h"
#include "Shader.h"

#include "Environment/HDRTexture.h"
#include "Environment/Cubemap.h"
#include "Environment/HDRConverter.h"
#include "Environment/Skybox.h"


// -------------------- Establish globals --------------------

const unsigned int width = 1200;
const unsigned int height = 800;

struct LightingParams {
    float intensity = 1.0f;
    glm::vec3 position = glm::vec3(0.0f, 5.0f, 2.0f);
    glm::vec4 color = glm::vec4(1.0f, 0.97f, 0.92f, 1.0f);
    float ambient = 0.5f;

    // Blinn-Phong
    /*float specularStr = 0.5f;
    float shininess = 32.0f;*/

	// Cook-Torrance
    /*float metallic = 0.5f;
    float roughness = 0.5f;*/
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

static void renderModel(Model& model, Shader& shader, Camera& camera,
    const LightingParams& params, float angle) {
    shader.Activate();
    camera.Matrix(shader, "camMatrix");

    // Common uniforms
    glm::vec4 finalLightColor = params.color * params.intensity;
    shader.setVec3("camPos", camera.Position);
    shader.setVec4("lightColor", finalLightColor);
    shader.setVec3("lightPos", params.position);
    shader.setFloat("ambient", params.ambient);

    /*model.setRotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));*/
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

	// Load HDR texture for environment mapping
    HDRTexture hdri("Environment/environment.hdr");
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

    // ------------ Load Models ------------
    std::cout << "Loading models..." << std::endl;

	// attempt to load model
    float t0 = (float)glfwGetTime();
    Model myModel("Models/robot-2020/robo.fbx");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] Model took " << (t1 - t0) << "s\n";

	
	myModel.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    myModel.setScale(glm::vec3(0.01f));


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
        renderModel(myModel, sceneShader, camera, lightingParams, angle);

		// Render skybox last
		skybox.Draw(camera);

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
    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}