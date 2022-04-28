#pragma once
#include "../../imgui/imgui.h"
#include <string>

class Window
{
public:
    Window(const std::string& name) : window_name(name) {  }
    virtual ~Window() {  }

    virtual void render() = 0;

    void finalRender()
    {
        if(open && ImGui::Begin(window_name.c_str(), &open, 0))
        {
            render();
            ImGui::End();
        }
    }

protected:
    bool open = false;
    std::string window_name;
};
