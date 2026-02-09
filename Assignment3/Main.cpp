/*
* Author: Priyansh Nayak
* Project: Test
* Course: CS7GV5: Real-Time Animation
*/

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <engine/Camera.h>
#include <engine/MathUtils.h>
#include <engine/Model.h>
#include <engine/Shader.h>

// skybox
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
    // Light parameters
    float intensity = 1.0f;
    glm::vec3 direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
    glm::vec4 color = glm::vec4(1.0f, 0.97f, 0.92f, 1.0f);
    float ambient = 0.25f;

    // Aircraft Euler state (degrees)
    float pitchDeg = 0.0f; // X
    float yawDeg   = 0.0f; // Y
    float rollDeg  = 0.0f; // Z
    float rotSpeed = 90.0f;

    bool forceGimbalLock = false;
    bool useQuaternionMode = false;
    bool useKeyframes = false;
};

struct Keyframe {
    glm::vec3 position;
    glm::quat rotation;
    float time; // seconds
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
    ImGui::DragFloat3("Light Direction", &params.direction.x, 0.1f);
    
    ImGui::Separator();
    ImGui::Text("Aircraft Rotation (Euler)");
    ImGui::DragFloat("Pitch", &params.pitchDeg, 0.1f, -90.0f, 90.0f);
    ImGui::DragFloat("Yaw",   &params.yawDeg,   0.1f);
    ImGui::DragFloat("Roll",  &params.rollDeg,  0.1f);
    ImGui::SliderFloat("Rot Speed", &params.rotSpeed, 10.0f, 360.0f);

    ImGui::Separator();
    ImGui::Checkbox("Force Gimbal Lock (Pitch = 89.9°)", &params.forceGimbalLock);
    ImGui::TextWrapped(
        "When enabled, pitch is locked near 90 degrees. "
        "Yaw and roll will collapse onto the same axis."
    );
    ImGui::Checkbox("Use Quaternion Mode", &params.useQuaternionMode);

    ImGui::Separator();
    ImGui::Checkbox("Use Keyframed Animation", &params.useKeyframes);

    ImGui::End();
}

// -------------------- Render Model --------------------

static void renderModel(Model& model, Shader& shader, Camera& camera,
    TweakableParams& params) {
    shader.Activate();
    camera.Matrix(shader, "camMatrix");

    // Controllable uniforms
    shader.setVec3("camPos", camera.Position);
    shader.setVec4("lightColor", params.color * params.intensity);
    shader.setVec3("lightDir", params.direction);
    shader.setFloat("ambient", params.ambient);

    model.Draw(shader);
}

static void updateAircraftRotation(GLFWwindow* window, Model& model,
    TweakableParams& params, float dt, glm::quat& aircraftQuat) {
    const RotationOrder order = RotationOrder::YXZ;

    // ---------------- Euler mode ----------------
    if (!params.useQuaternionMode) {
        float change = params.rotSpeed * dt;

        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) params.pitchDeg += change;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) params.pitchDeg -= change;

        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) params.yawDeg += change;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) params.yawDeg -= change;

        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) params.rollDeg += change;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) params.rollDeg -= change;

        // Gimbal lock demo
        if (params.forceGimbalLock) params.pitchDeg = 89.9f;
        // Clamp pitch to avoid singularity in Euler angles
        params.pitchDeg = glm::clamp(params.pitchDeg, -89.9f, 89.9f);

        model.setRotationEuler(
            params.pitchDeg,
            params.yawDeg,
            params.rollDeg,
            order
        );
    }
    // ---------------- Quaternion mode ----------------
    else {
        float angle = glm::radians(params.rotSpeed * dt);

        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
            aircraftQuat = glm::angleAxis( angle, glm::vec3(1,0,0)) * aircraftQuat;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            aircraftQuat = glm::angleAxis(-angle, glm::vec3(1,0,0)) * aircraftQuat;

        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
            aircraftQuat = glm::angleAxis( angle, glm::vec3(0,1,0)) * aircraftQuat;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
            aircraftQuat = glm::angleAxis(-angle, glm::vec3(0,1,0)) * aircraftQuat;

        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
            aircraftQuat = glm::angleAxis( angle, glm::vec3(0,0,1)) * aircraftQuat;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
            aircraftQuat = glm::angleAxis(-angle, glm::vec3(0,0,1)) * aircraftQuat;

        aircraftQuat = glm::normalize(aircraftQuat);
        model.setRotationQuat(aircraftQuat);
    }
}

static void updateAircraftFromKeyframes(Model& model, float& animTime, float dt,
    const std::vector<Keyframe>& keys) {
    // Need at least two keyframes to interpolate
    if (keys.size() < 2) return;
    // Store previous state for velocity calculation
    static glm::vec3 prevPos = keys[0].position;
    static glm::quat prevRot = keys[0].rotation;

    // Advance animation time and loop back to start when we reach the end
    animTime += dt;
    float duration = keys.back().time;
    animTime = fmod(animTime, duration);
    
    // Find the current keyframe interval [a, b]
    int i = 0;
    while (i + 2 < (int)keys.size() - 1 && animTime > keys[i + 1].time) ++i;
    i = glm::clamp(i, 1, (int)keys.size() - 3); // Ensure we have k0 and k3 for Catmull-Rom

    const Keyframe& k0 = keys[i - 1];
    const Keyframe& k1 = keys[i];
    const Keyframe& k2 = keys[i + 1];
    const Keyframe& k3 = keys[i + 2];
    
    // Normalized time between keyframes [0, 1]
    float t = (animTime - k1.time) / (k2.time - k1.time);
    // Easing for smoother motion
    if (animTime < duration - 0.2f) t = MathUtils::easeInOut(t);

    // Interpolate position with Catmull-Rom spline
    glm::vec3 pos = MathUtils::catmullRom(k0.position, k1.position, k2.position, k3.position, t);

    // velocity for look rotation
    glm::vec3 velocity = pos - prevPos;
    glm::quat targetRot = prevRot;
    if (glm::length(velocity) > 0.001f) {
        glm::vec3 forward = glm::normalize(velocity);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        // Stable look rotation
        glm::mat4 look = glm::lookAt(glm::vec3(0.0f), -forward, up);
        targetRot = glm::quat_cast(glm::inverse(look));
    }

    // Interpolate rotation with SLERP
    glm::quat rot = MathUtils::slerp(prevRot, targetRot, 0.15f);

    // Apply interpolated transform to model
    model.setPosition(pos);
    model.setRotationQuat(rot);
    prevPos = pos;
    prevRot = rot;
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
    sceneShader.Activate();
    sceneShader.setBool("useTexture", true);
    sceneShader.setInt("diffuse0", 0);
    sceneShader.setInt("specular0", 1);

    Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");

    // ------------ Load Models ------------
    std::cout << "Loading models..." << std::endl;

	// attempt to load model
    float t0 = (float)glfwGetTime();
    Model plane("Models/plane.obj");
    float t1 = (float)glfwGetTime();
    std::cout << "[Load] Model took " << (t1 - t0) << "s\n";

	plane.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    plane.setScale(glm::vec3(0.01f));

    // Figure-of-eight Catmull–Rom keyframes
    std::vector<Keyframe> keyframes = {
        // Control point BEFORE start
        { glm::vec3( 0.0f,  0.0f,  0.0f), glm::quat(), -2.0f },

        // Actual animation starts here
        { glm::vec3( 0.0f,  0.0f,  0.0f), glm::quat(), 0.0f },

        // Left loop
        { glm::vec3(-10.0f,  4.0f, 5.0f), glm::quat(), 2.0f },
        { glm::vec3(-10.0f,  -4.0f, -5.0f), glm::quat(), 4.0f },

        // Back through center
        { glm::vec3( 0.0f,  0.0f, 0.0f), glm::quat(), 6.0f },

        // Right loop
        { glm::vec3( 10.0f,  4.0f, 5.0f), glm::quat(), 8.0f },
        { glm::vec3( 10.0f,  -4.0f, -5.0f), glm::quat(), 10.0f },

        // End at center
        { glm::vec3( 0.0f,  0.0f,  0.0f), glm::quat(), 12.0f },

        // Control point AFTER end
        { glm::vec3( 0.0f,  0.0f,  0.0f), glm::quat(), 13.0f }
    };

    // ------------ Render Loop ------------
    TweakableParams params;
    float prevTime = (float)glfwGetTime();
	bool pWasDown = true;
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    float animTime = 0.0f;
    glm::quat aircraftQuat = glm::quat(1, 0, 0, 0);
	std::cout << "Entering render loop..." << std::endl;
    // this loop will run until we close window
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - prevTime;
        prevTime = now;

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
        
        // Render the model with current parameters
        if (params.useKeyframes) {
            updateAircraftFromKeyframes(plane, animTime, dt, keyframes);
        } else {
            updateAircraftRotation(window, plane, params, dt, aircraftQuat);
        }
        renderModel(plane, sceneShader, camera, params);

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
    skyboxShader.Delete();
    // deletes window before ending program
    glfwDestroyWindow(window);
    // terminate GLFW before ending program
    glfwTerminate();


    return 0;

}