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
    // basic lighting
    shaderProgram.setVec4("lightColor", glm::vec4(1.0f, 0.97f, 0.92f, 1.0f));
    shaderProgram.setVec3("lightPos", glm::vec3(1.0f, 4.0f, 5.0f));


	// attempt to load teapot model
    float t0 = (float)glfwGetTime();
	Model teapot("Models/japanese_teapot.glb");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] teapot took " << (t1 - t0) << "s\n";

    teapot.setScale(glm::vec3(0.05f));
    teapot.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));


    // ------------ Render Loop ------------
    float prevTime = (float)glfwGetTime();

    // this loop will run until we close window
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - prevTime;
        prevTime = now;

		camera.Inputs(window, dt);

        // Updates and exports the camera matrix to the Vertex Shader
        camera.updateMatrix(0.5f, 100.0f);
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