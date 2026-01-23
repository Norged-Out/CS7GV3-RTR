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


    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");


	// ------------ Load Assets ------------

    // setup shaders
    Shader blinnPhongShader("Shaders/scene.vert", "Shaders/blinnPhong.frag");
    blinnPhongShader.Activate();
    blinnPhongShader.setInt("diffuse0", 0);
    blinnPhongShader.setInt("specular0", 1);

	Shader toonShader("Shaders/scene.vert", "Shaders/toon.frag");
	toonShader.Activate();
	toonShader.setInt("diffuse0", 0);
	toonShader.setInt("specular0", 1);

    // UI-controlled lighting parameters
    float ambient = 0.4f;
    float specularStr = 0.5f;
    float shininess = 32.0f;
    int toonLevels = 3;
    glm::vec3 lightPos = glm::vec3(0.0f, 3.0f, 4.0f);
    glm::vec4 lightColor = glm::vec4(1.0f, 0.97f, 0.92f, 1.0f);


	// attempt to load teapot model
    float t0 = (float)glfwGetTime();
	Model teapot1("Models/clay-teapot/teapot.fbx",
                  "Models/clay-teapot/teapot_BaseColor.png",
                  "Models/clay-teapot/teapot_Roughness.png");
    Model teapot2("Models/clay-teapot/teapot.fbx",
                  "Models/clay-teapot/teapot_BaseColor.png",
                  "Models/clay-teapot/teapot_Roughness.png");
    Model teapot3("Models/clay-teapot/teapot.fbx",
                  "Models/clay-teapot/teapot_BaseColor.png",
                  "Models/clay-teapot/teapot_Roughness.png");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] teapots took " << (t1 - t0) << "s\n";

    
    // Teapot 1 - Center (Blinn-Phong)
    teapot1.setScale(glm::vec3(0.01f));
    teapot1.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    // Teapot 2 - Left (Toon)
    teapot2.setScale(glm::vec3(0.01f));
    teapot2.setPosition(glm::vec3(-4.0f, 0.0f, 0.0f));

    // Teapot 3 - Right (PBR)
    teapot3.setScale(glm::vec3(0.01f));
    teapot3.setPosition(glm::vec3(4.0f, 0.0f, 0.0f));


    // point camera at teapot
    glm::vec3 target(0.0f, 0.0f, 0.0f);   // teapot at origin

    camera.Position = glm::vec3(0.0f, 2.0f, 4.0f);   // back a bit, slightly up
    glm::vec3 dir = glm::normalize(target - camera.Position);
    camera.Orientation = dir;

    // sync yaw/pitch
    camera.pitch = glm::degrees(asin(dir.y));
    camera.yaw = glm::degrees(atan2(dir.z, dir.x));


    // ------------ Render Loop ------------
    float prevTime = (float)glfwGetTime();
	bool pWasDown = false;
    float rotationSpeed = 20.0f;
	float angle = 0.0f;
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

        // ImGui UI Window
        ImGui::Begin("Lighting Controls");
        ImGui::Text("Adjust lighting parameters:");
        ImGui::Separator();
        ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular Strength", &specularStr, 0.0f, 2.0f);
        ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f);
        ImGui::SliderInt("Toon Levels", &toonLevels, 2, 5);
        ImGui::Separator();
        ImGui::ColorEdit3("Light Color", &lightColor.r);
        ImGui::DragFloat3("Light Position", &lightPos.x, 0.1f);
        ImGui::End();

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

        camera.UpdateWithMode(window, dt);

        // Updates and exports the camera matrix to the Vertex Shader
        camera.updateMatrix(0.5f, 100.0f);       
     
		// draw teapot 1 (Blinn-Phong)
        blinnPhongShader.Activate();
        camera.Matrix(blinnPhongShader, "camMatrix");
        blinnPhongShader.setVec3("camPos", camera.Position);
        blinnPhongShader.setVec4("lightColor", lightColor);
        blinnPhongShader.setVec3("lightPos", lightPos);
        blinnPhongShader.setFloat("ambient", ambient);
        blinnPhongShader.setFloat("specularStr", specularStr);
        blinnPhongShader.setFloat("shininess", shininess);
        teapot1.setRotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        teapot1.Draw(blinnPhongShader);

		// draw teapot 2 (Toon)
		toonShader.Activate();
		camera.Matrix(toonShader, "camMatrix");
		toonShader.setVec3("camPos", camera.Position);
		toonShader.setVec4("lightColor", lightColor);
		toonShader.setVec3("lightPos", lightPos);
		toonShader.setFloat("ambient", ambient);
		toonShader.setFloat("specularStr", specularStr);
		toonShader.setFloat("shininess", shininess);
		toonShader.setInt("toonLevels", toonLevels);
        teapot2.setRotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        teapot2.Draw(toonShader);


        
		teapot3.setRotation(angle, glm::vec3(0.0f, 1.0f, 0.0f));
        
		
		teapot3.Draw(blinnPhongShader);

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
    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}