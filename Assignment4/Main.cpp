/*
* Author: Priyansh Nayak
* Project: Mipmaps
* Course: CS7GV3: Real-Time Rendering
*/

#include <iostream>
#include <engine/AppSetup.h>
#include <engine/Camera.h>
#include <engine/Texture.h>
#include <engine/Material.h>
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

    // Texture parameters    
    bool useTextures = true;
    int minFilterMode = 5; // default trilinear
    int magFilterMode = 1; // default linear
    int wrapMode = 0;      // default repeat
};

// -------------------- GUI Setup --------------------

static void buildGUI(TweakableParams& params) {
    ImGui::Begin("Controls");
    ImGui::SliderFloat("Light Intensity", &params.intensity, 0.5f, 5.0f);
    ImGui::SliderFloat("Ambient", &params.ambient, 0.0f, 1.0f);
    ImGui::ColorEdit3("Light Color", &params.color.r);
    ImGui::DragFloat3("Light Position", &params.position.x, 0.1f);

    ImGui::Separator();
    ImGui::Checkbox("Orbit Light", &params.orbitLight);
    ImGui::SliderFloat("Orbit Radius", &params.orbitRadius, 1.0f, 10.0f);
    ImGui::SliderFloat("Orbit Speed", &params.orbitSpeed, 0.1f, 1.0f);

    ImGui::Separator();
    ImGui::Text("Texture Sampling");    
    ImGui::Checkbox("Use Textures", &params.useTextures);

    // Minification filter options
    const char* minModes[] = {
        "Nearest (No Mipmaps)",
        "Linear (No Mipmaps)",
        "Nearest Mipmap Nearest",
        "Linear Mipmap Nearest",
        "Nearest Mipmap Linear",
        "Trilinear (Linear Mipmap Linear)"
    };
    ImGui::Combo("Min Filter", &params.minFilterMode, minModes, IM_ARRAYSIZE(minModes));

    // Magnification filter options
    const char* magModes[] = {
        "Nearest",
        "Linear"
    };
    ImGui::Combo("Mag Filter", &params.magFilterMode, magModes, IM_ARRAYSIZE(magModes));

    // Wrap mode options
    const char* wrapModes[] = {
        "Repeat",
        "Mirrored Repeat",
        "Clamp to Edge"
    };
    ImGui::Combo("Wrap Mode", &params.wrapMode, wrapModes, IM_ARRAYSIZE(wrapModes));

    ImGui::End();
}

// -------------------- Texture Controls --------------------

void updateTextureParams(TweakableParams& params, 
    const std::shared_ptr<Material> mat) {
    static int prevMin = -1;
    static int prevMag = -1;
    static int prevWrap = -1;

    // Guard against no change
    if (params.minFilterMode == prevMin && params.magFilterMode == prevMag && params.wrapMode == prevWrap)
        return;
    
    GLenum minFilter, magFilter, wrapMode;
    switch(params.minFilterMode) {
        case 0: minFilter = GL_NEAREST; break;
        case 1: minFilter = GL_LINEAR; break;
        case 2: minFilter = GL_NEAREST_MIPMAP_NEAREST; break;
        case 3: minFilter = GL_LINEAR_MIPMAP_NEAREST; break;
        case 4: minFilter = GL_NEAREST_MIPMAP_LINEAR; break;
        case 5: minFilter = GL_LINEAR_MIPMAP_LINEAR; break;
        default: minFilter = GL_LINEAR_MIPMAP_LINEAR;
    }
    switch(params.magFilterMode) {
        case 0: magFilter = GL_NEAREST; break;
        case 1: magFilter = GL_LINEAR; break;
        default: magFilter = GL_LINEAR;
    }
    switch(params.wrapMode) {
        case 0: wrapMode = GL_REPEAT; break;
        case 1: wrapMode = GL_MIRRORED_REPEAT; break;
        case 2: wrapMode = GL_CLAMP_TO_EDGE; break;
        default: wrapMode = GL_REPEAT;
    }
    
    mat->setFiltering(minFilter, magFilter);
    mat->setWrapping(wrapMode, wrapMode);

    prevMin = params.minFilterMode;
    prevMag = params.magFilterMode;
    prevWrap = params.wrapMode;
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

    // Toggles
    shader.setBool("useTextures", params.useTextures);

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
    std::cout << "Assignment 4: Mipmaps" << std::endl;

    // ------------ Initialize the Window ------------

    // create a window
    GLFWwindow* window = initWindow(width, height, "Assignment 4: Mipmaps");
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
    Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");

    // ------------ Setup Geometry ------------

    sceneShader.Activate();

    std::cout << "Initializing custom meshes..." << std::endl;
    
    auto lightGizmo = Geometry::createSphereMesh(16, 16, false);
    auto cube = Geometry::createCubeMesh();
    auto sphere = Geometry::createSphereMesh();
    auto pyramid = Geometry::createPyramidMesh();
    auto plane = Geometry::createPlaneMesh(200.0f, 200.0f, 100.0f);

    auto pebbleMat = Material::CreateMat(
        "Textures/pebble/diffuse.png",
        "Textures/pebble/normal.png",
        "Textures/pebble/rough.png"
    );
    cube->setMaterial(pebbleMat);
    sphere->setMaterial(pebbleMat);
    pyramid->setMaterial(pebbleMat);
    plane->setMaterial(pebbleMat);

    float spacing = 3.5f;
    glm::mat4 cubeModel(1.0f);
    glm::mat4 sphereModel(1.0f);
    glm::mat4 pyramidModel(1.0f);

    glm::mat4 planeModel = MathUtils::buildTRS(
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(1,0,0), 0.0f,
        glm::vec3(1.0f)
    );

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

        
        updateTextureParams(params, pebbleMat);

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

        // Render plane
        renderMesh(*plane, sceneShader, camera, params, planeModel);

        // Build model matrices for rotating objects
        cubeModel = MathUtils::buildTRS(
            glm::vec3(-spacing, 1.0f, 0.0f), 
            glm::vec3(0.5,1,-0.5), angle, 
            glm::vec3(1.5f)
        );
        sphereModel = MathUtils::buildTRS(
            glm::vec3(0.0f, 1.0f, 0.0f), 
            glm::vec3(0,-1,0), angle, 
            glm::vec3(1.0f)
        );
        pyramidModel = MathUtils::buildTRS(
            glm::vec3(spacing, 1.0f, 0.0f), 
            glm::vec3(0,1,0), angle, 
            glm::vec3(2.0f)
        );

        renderLightGizmo(*lightGizmo, lightShader, camera, params);
        renderMesh(*cube, sceneShader, camera, params, cubeModel);
        renderMesh(*sphere, sceneShader, camera, params, sphereModel);
        renderMesh(*pyramid, sceneShader, camera, params, pyramidModel);

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
    // delete shader program
    sceneShader.Delete();
    lightShader.Delete();
    skyboxShader.Delete();

    shutdownImGui();
    shutdownWindow(window);
    
    return 0;
}