#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 earthPosition = glm::vec3(0.0f, -3.5f, 0.0f);
    float earthScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n';
        //<< camera.Position.x << '\n'
        //<< camera.Position.y << '\n'
        //<< camera.Position.z << '\n'
        //<< camera.Front.x << '\n'
        //<< camera.Front.y << '\n'
        //<< camera.Front.z << '\n';




}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;


void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");

    // load models
    // -----------
    Model earthModel("resources/objects/earth/Earth.obj");
    Model mercuryModel("resources/objects/mercury/Mercury_1K.obj");
    Model sunModel("resources/objects/sun/sun.obj");
    Model venusModel("resources/objects/venus/jupiter.obj");

    earthModel.SetShaderTextureNamePrefix("material.");
    mercuryModel.SetShaderTextureNamePrefix("material.");
    sunModel.SetShaderTextureNamePrefix("material.");
    venusModel.SetShaderTextureNamePrefix("material.");

    glm::vec3 sunPosition = glm::vec3(15.0f, -1.0f, 0.0f);
    programState->pointLight.position = sunPosition;



    PointLight& pointLight = programState->pointLight;
    pointLight.position = sunPosition;
    pointLight.ambient = glm::vec3(0.5, 0.5, 0.5);
    pointLight.diffuse = glm::vec3(2.5, 2.5, 2.5);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        //pointLight.position = glm::vec3(15.0f * cos(currentFrame), -1.0f, 0.0 * sin(currentFrame));
        pointLight.position = glm::vec3(15.0f, -1.0f, 0.0 );
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);


        glm::vec3 sunPosition = glm::vec3(15.0f, -3.5f, 0.0f);

        float earthOrbitRadius = 18.0f;
        float earthAngle = glm::radians(glfwGetTime() * 10.0f);
        glm::vec3 earthPosition = glm::vec3(cos(earthAngle) * earthOrbitRadius + sunPosition.x, sunPosition.y, sin(earthAngle) * earthOrbitRadius + sunPosition.z);
        glm::mat4 model1 = glm::mat4(1.0f);
        model1 = glm::translate(model1, earthPosition);
        model1 = glm::scale(model1, glm::vec3(1.0f));
        ourShader.setMat4("model", model1);
        earthModel.Draw(ourShader);

        float mercuryOrbitRadius = 11.0f;
        float mercuryAngle = glm::radians(glfwGetTime() * 20.0f);
        glm::vec3 mercuryPosition = glm::vec3(cos(mercuryAngle) * mercuryOrbitRadius + sunPosition.x, sunPosition.y, sin(mercuryAngle) * mercuryOrbitRadius + sunPosition.z);
        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, mercuryPosition);
        model2 = glm::scale(model2, glm::vec3(2.0f));
        ourShader.setMat4("model", model2);
        mercuryModel.Draw(ourShader);

        float venusOrbitRadius = 13.0f;
        float venusAngle = glm::radians(glfwGetTime() * 15.0f);
        glm::vec3 venusPosition = glm::vec3(cos(venusAngle) * venusOrbitRadius + sunPosition.x, sunPosition.y, sin(venusAngle) * venusOrbitRadius + sunPosition.z);
        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4, venusPosition);
        model4 = glm::scale(model4, glm::vec3(1.0f));
        ourShader.setMat4("model", model4);
        venusModel.Draw(ourShader);

        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3, glm::vec3(15.0f, -3.5f, 0.0f));
        model3 = glm::scale(model3, glm::vec3(5.0f));
        model3 = glm::rotate(model3, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model3);
        sunModel.Draw(ourShader);


        /*
        // EARTH
        glm::mat4 model1 = glm::mat4(1.0f);
        model1 = glm::translate(model1, programState->earthPosition);
        model1 = glm::scale(model1, glm::vec3(1.0f));
        ourShader.setMat4("model", model1);
        earthModel.Draw(ourShader);

        // MERCURY
        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, glm::vec3(10.0f, -3.5f, 0.0f));
        model2 = glm::scale(model2, glm::vec3(2.0f));
        model2 = glm::rotate(model2, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model2);
        mercuryModel.Draw(ourShader);

        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4, glm::vec3(5.0f, -3.5f, 0.0f));
        model4 = glm::scale(model4, glm::vec3(1.0f));
        model4 = glm::rotate(model4, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setMat4("model", model4);
        venusModel.Draw(ourShader);

        */


        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //{
        //static float f = 0.0f;
        //ImGui::Begin("Hello window");
        //ImGui::Text("Hello text");
        //ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        //ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        //ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        //ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        //ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        //ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        //ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        //ImGui::End();
    //}

    //{
        //ImGui::Begin("Camera info");
        //const Camera& c = programState->camera;
        //ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        //ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        //ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        //ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        //ImGui::End();
    //}

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}
