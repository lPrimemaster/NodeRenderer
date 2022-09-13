#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "log/logger.h"

#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"

#include "windows/window_manager.inl"

#include "render/renderer.h"

#include "python/loader.h" // For init and deinit

#include "util/serialization.inl"

#include "math/kdtree.inl"

static constexpr int DEF_SCREEN_PX_W = 1280;
static constexpr int DEF_SCREEN_PX_H =  720;

int main(int argc, char* argv[])
{
    PythonLoader::Init();
    Py_BEGIN_ALLOW_THREADS

    GLFWwindow* window;

    if (!glfwInit())
    {
        L_ERROR("glfwInit() error.");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

    window = glfwCreateWindow(DEF_SCREEN_PX_W, DEF_SCREEN_PX_H, "CBG", NULL, NULL);
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
    glfwWindowHint(GLFW_SAMPLES, 4);

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
    AnalyticsWindow* analyticsWindow = WindowManager::GetAnalyticsWindow();
    OptionsWindow* optionsWindow = WindowManager::GetOptionsWindow();
    Renderer::DrawList dl(window, DEF_SCREEN_PX_W, DEF_SCREEN_PX_H);
    nodeWindow->setDrawActiveList(&dl);

    nodeWindow->setWindowSize(ImVec2((float)DEF_SCREEN_PX_W, (float)DEF_SCREEN_PX_H / 3.0f));
    nodeWindow->setWindowPos(ImVec2(0, 0));

    analyticsWindow->setWindowSize(ImVec2(AnalyticsWindow::WIDTH, (float)DEF_SCREEN_PX_H / 3.0f));
    analyticsWindow->setWindowPos(ImVec2(0, (float)DEF_SCREEN_PX_H - 19.0f));
    analyticsWindow->setCollapsedPosY((float)DEF_SCREEN_PX_H - 19.0f);

    optionsWindow->setWindowSize(ImVec2(OptionsWindow::WIDTH, (float)DEF_SCREEN_PX_H / 3.0f));
    optionsWindow->setWindowPos(ImVec2(AnalyticsWindow::WIDTH, (float)DEF_SCREEN_PX_H - 19.0f));
    optionsWindow->setCollapsedPosY((float)DEF_SCREEN_PX_H - 19.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    

    // TODO: Basic model needs to change to apply right backface culling
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Render
        dl.render(window, nodeWindow, analyticsWindow, optionsWindow);

        // Render the UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        windowManager.renderAll();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        //glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    Py_END_ALLOW_THREADS
    PythonLoader::Deinit();

    return 0;
}
