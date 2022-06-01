#pragma once
#include "../../imgui/imgui.h"
#include <string>

class Window
{
public:
    constexpr Window(const char* name, const bool closeable) : window_name(name), closeable(closeable) {  }
    virtual ~Window() {  }

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

    void finalRender()
    {
        update();
        ImGui::SetNextWindowSize(windowSize);
        ImGui::SetNextWindowPos(windowPos);
        if(closeable)
        {
            if(open && ImGui::Begin(window_name, &open, window_flags))
            {
                render();
                ImGui::End();
            }
        }
        else
        {
            if(ImGui::Begin(window_name, nullptr, window_flags))
            {
                render();
                ImGui::End();
            }
        }
    }

protected:
    bool open = false;
    const char* window_name;
    ImGuiWindowFlags window_flags = 0;
    const bool closeable;
    
private:
    ImVec2 windowSize;
    ImVec2 windowPos;
};
