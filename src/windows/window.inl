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
    }

protected:
    bool open = false;
    bool collapsed = false;
    const char* window_name;
    ImGuiWindowFlags window_flags = 0;
    const bool closeable;
    
private:
    ImVec2 windowSize;
    ImVec2 windowPos;
};
