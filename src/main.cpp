#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "log/logger.h"

#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

#include "windows/window_manager.inl"

#include "render/renderer.h"

int main(int argc, char* argv[])
{
    GLFWwindow* window;

    if (!glfwInit())
    {
        L_ERROR("glfwInit() error.");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    window = glfwCreateWindow(1280, 720, "CBG", NULL, NULL);
    if (!window)
    {
        L_ERROR("glfwCreateWindow() error.");
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        L_ERROR("gladLoadGLLoader() error.");
        glfwTerminate();
        return -1;
    }
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    WindowManager& windowManager = WindowManager::Instance();
    NodeWindow* nodeWindow = WindowManager::GetNodeWindow();
    Renderer::DrawList dl;

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Render to the screen
        // TODO: Render to a texture and display it on an ImGui window
        dl.render(nodeWindow);

        // Render the UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        windowManager.renderAll();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
