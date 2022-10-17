#pragma once
#include <vector>
#include "../../imgui/imgui.h"
#include "window.inl"
#include "../util/updateclient.h"
#include <future>
#include <GLFW/glfw3.h>

class UpdateCheckWindow : public Window
{
public:
    UpdateCheckWindow(const char* name) : Window(name, false)
    {
        open = true;
        setWindowCollapsed(false);
        window_flags = 
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoResize        |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse      |
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoTitleBar      |
            ImGuiWindowFlags_NoBackground;

        // Check for updates
        new_update = IsUpdateAvailable(&udata);

        if(new_update)
        {
            L_DEBUG("UpdateCheckWindow :: There seems to be a new version of the app.");
        }
        else
        {
            L_DEBUG("UpdateCheckWindow :: Failed to find new updates.");
        }
    }

    ~UpdateCheckWindow()
    {
        L_DEBUG("~UpdateCheckWindow()");
    }

    void setGlfwWindowHandle(GLFWwindow* window)
    {
        glfw_window_handle = window;
    }

    virtual void render() override;
    virtual void update() override;

private:
    std::future<void> dl_future; 
    std::atomic<size_t> current_download_size = 0;
    bool updating = false;
    bool popup_open = false;
    bool new_update = false;
    std::string download_url;
    UpdateData udata;

    GLFWwindow* glfw_window_handle;

    const ImVec2 window_size = ImVec2(200, 50);
};
