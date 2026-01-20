#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>

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

    // register callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ------------ Render Loop ------------
    float prevTime = (float)glfwGetTime();

    // this loop will run until we close window
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - prevTime;
        prevTime = now;

        // unbind the VAO
        glBindVertexArray(0);
        // swap front and back buffers
        glfwSwapBuffers(window);
        // take care of all GLFW events
        glfwPollEvents();

    }

    // ------------ Clean up ------------

    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}