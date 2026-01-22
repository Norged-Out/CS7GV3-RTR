#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include "Camera.h"
#include "Model.h"
#include "Shader.h"

// -------------------- Establish globals --------------------

const unsigned int width = 1200;
const unsigned int height = 800;

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

// -------------------- Main --------------------

int main() {
    std::cout << "Assignment 1 start!" << std::endl;

    // ------------ Initialize the Window ------------

    // create a window of 800x800 size
    GLFWwindow* window = initWindow(width, height, "Assignment 1");

    // sanity check for smooth camera motion
    glfwSwapInterval(1);

    // use GLAD to configure OpenGL
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return -1;
    }
    // specify window dimensions
    glViewport(0, 0, width, height);

    // Creates camera object
    Camera camera(width, height, glm::vec3(0.0f, 0.0f, 2.0f));
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

    // Enable depth and backface culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // setup shaders
    Shader shaderProgram("Shaders/scene.vert", "Shaders/scene.frag");
    shaderProgram.Activate();
    shaderProgram.setInt("diffuse0", 0);
    shaderProgram.setInt("specular0", 1);

    // Lighting
    shaderProgram.setVec4("lightColor", glm::vec4(1.0f, 0.97f, 0.92f, 1.0f));
    shaderProgram.setVec3("lightPos", glm::vec3(0.0f, 2.0f, 3.0f));

    // Material properties
    shaderProgram.setFloat("ambient", 0.5f);
    shaderProgram.setFloat("specularStr", 0.5f);
    shaderProgram.setFloat("shininess", 16.0f);
    shaderProgram.setFloat("uvScale", 1.0f);



	// attempt to load teapot model
    float t0 = (float)glfwGetTime();
	Model teapot("Models/clay-teapot/teapot.fbx",
                 "Models/clay-teapot/teapot_BaseColor.png",
                 "Models/clay-teapot/teapot_Roughness.png");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] teapot took " << (t1 - t0) << "s\n";

    teapot.setScale(glm::vec3(0.01f));
    teapot.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    // point camera at teapot
    glm::vec3 target(0.0f, 0.0f, 0.0f);   // teapot at origin

    camera.Position = glm::vec3(0.0f, 2.0f, 4.0f);   // back a bit, slightly up
    glm::vec3 dir = glm::normalize(target - camera.Position);
    camera.Orientation = dir;

    // sync yaw/pitch
    camera.pitch = glm::degrees(asin(dir.y));
    camera.yaw = glm::degrees(atan2(dir.z, dir.x));


    glm::vec3 bbMin = teapot.getAABBMin();
    glm::vec3 bbMax = teapot.getAABBMax();

    glm::vec3 centerXZ(
        (bbMin.x + bbMax.x) * 0.0f,
        (bbMin.y + bbMax.y) * 0.0f,
        (bbMin.z + bbMax.z) * 0.5f
    );



    // ------------ Render Loop ------------
    float prevTime = (float)glfwGetTime();
	bool pWasDown = false;
    // this loop will run until we close window
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - prevTime;
        prevTime = now;

        bool pDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pDown && !pWasDown) {
            camera.ToggleCinema(target);
        }
        pWasDown = pDown;

        camera.UpdateWithMode(window, dt);

        // Updates and exports the camera matrix to the Vertex Shader
        camera.updateMatrix(0.5f, 100.0f);
        shaderProgram.Activate();
        shaderProgram.setVec3("camPos", camera.Position);
		camera.Matrix(shaderProgram, "camMatrix");

        // clear the screen and specify background color
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        // clean back buffer and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        teapot.Draw(shaderProgram);

        // unbind the VAO
        glBindVertexArray(0);
        // swap front and back buffers
        glfwSwapBuffers(window);
        // take care of all GLFW events
        glfwPollEvents();

    }

    // ------------ Clean up ------------

	shaderProgram.Delete();

    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}