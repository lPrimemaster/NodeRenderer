#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include "../../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../log/logger.h"
#include <string>

class Window
{
public:
    constexpr Window(const char* name, const bool closeable) : window_name(name), closeable(closeable) {  }
    virtual ~Window() 
    {
        L_DEBUG("~Window()");
        cleanup_floating();
    }

    virtual void render() = 0;
    virtual void update() = 0;

    inline void setWindowSize(ImVec2 windowSize)
    {
        this->windowSize = windowSize;
    }

    inline void setWindowPos(ImVec2 windowPos)
    {
        this->windowPos = windowPos;
    }

    inline void setWindowCollapsed(bool collapsed)
    {
        this->collapsed = collapsed;
    }

    inline ImVec2 getWindowSize() const
    {
        return windowSize;
    }

    inline bool isCollapsed() const
    {
        return collapsed;
    }

    void finalRender()
    {
        update();

        GLFWwindow* old_window_ctx;
        ImGuiContext* old_imgui_ctx;
        bool swap_frame_here = false;

        if(floating_w)
        {
            swap_frame_here = true;
            floating_last_frame = true;
            // The window is floating
            // Create a new ImGui context and an OS window for it
            old_window_ctx = glfwGetCurrentContext();
            old_imgui_ctx = ImGui::GetCurrentContext();
            if(!new_context_created)
            {
                // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
                glfw_window = glfwCreateWindow(windowSize.x, windowSize.y, window_name, NULL, old_window_ctx);
                if (!glfw_window)
                {
                    L_ERROR("glfwCreateWindow() error.");
                    L_ERROR("Could not create a floating window.");
                }
                else
                {
                    glfwMakeContextCurrent(glfw_window);
                    glfwSwapInterval(1);
                    glfwWindowHint(GLFW_SAMPLES, 4);

                    imgui_ctx = ImGui::CreateContext();
                    ImGui::SetCurrentContext(imgui_ctx);
                    // ImGuiIO& io = ImGui::GetIO();

                    // Setup Dear ImGui style
                    ImGui::StyleColorsDark();

                    // Setup Platform/Renderer backends
                    ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
                    ImGui_ImplOpenGL3_Init("#version 130");

                    // glfwSetWindowFocusCallback(glfw_window, ImGui_ImplGlfw_WindowFocusCallback);
                    // glfwSetCursorEnterCallback(glfw_window, ImGui_ImplGlfw_CursorEnterCallback);
                    // glfwSetCursorPosCallback(glfw_window, ImGui_ImplGlfw_CursorPosCallback);
                    // glfwSetMouseButtonCallback(glfw_window, ImGui_ImplGlfw_MouseButtonCallback);
                    // glfwSetScrollCallback(glfw_window, ImGui_ImplGlfw_ScrollCallback);
                    // glfwSetKeyCallback(glfw_window, ImGui_ImplGlfw_KeyCallback);
                    // glfwSetCharCallback(glfw_window, ImGui_ImplGlfw_CharCallback);

                    new_context_created = true;
                }
            }

            glfwMakeContextCurrent(glfw_window);
            ImGui::SetCurrentContext(imgui_ctx);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }
        else if(floating_last_frame)
        {
            floating_last_frame = false;
            cleanup_floating();
        }

        ImGui::SetNextWindowSize(windowSize);
        ImGui::SetNextWindowPos(windowPos);
        ImGui::SetNextWindowCollapsed(collapsed);
        if(closeable)
        {
            collapsed = !ImGui::Begin(window_name, &open, window_flags);
            if(open && !collapsed)
            {
                render();
            }
            ImGui::End();
        }
        else
        {
            
            collapsed = !ImGui::Begin(window_name, nullptr, window_flags);
            if(!collapsed)
            {
                render();
            }
            ImGui::End();
        }

        if(swap_frame_here)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(glfw_window);
            // glfwPollEvents();

            glfwMakeContextCurrent(old_window_ctx);
            ImGui::SetCurrentContext(old_imgui_ctx);
        }
    }

protected:
    bool open = false;
    bool collapsed = false;
    const char* window_name;
    ImGuiWindowFlags window_flags = 0;
    const bool closeable;
    bool floating_w = false;
    
private:

    inline void cleanup_floating(bool from_other_ctx = false)
    {
        L_DEBUG("Attempting to destroy floating window!");
        if(new_context_created && glfw_window && imgui_ctx)
        {
            ImGuiContext* other_ctx = ImGui::GetCurrentContext();

            // Did we came from another ImGui context?
            bool reset_ctx_at_end = (other_ctx != imgui_ctx);

            L_DEBUG("Destroying floating window!");
            ImGui::SetCurrentContext(imgui_ctx);
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext(imgui_ctx);
            glfwDestroyWindow(glfw_window);

            new_context_created = false;
            glfw_window = nullptr;
            imgui_ctx = nullptr;

            if(reset_ctx_at_end)
            {
                ImGui::SetCurrentContext(other_ctx);
            }
        }
    }

    bool floating_last_frame = false;
    bool new_context_created = false;
    GLFWwindow* glfw_window = nullptr;
    ImGuiContext* imgui_ctx = nullptr;
    ImVec2 windowSize;
    ImVec2 windowPos;
};
