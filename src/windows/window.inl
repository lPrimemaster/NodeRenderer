#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include "../../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include "../log/logger.h"
#include <string>

inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }

class Window
{
public:
    constexpr Window(const char* name, const bool closeable) : window_name(name), closeable(closeable) {  }
    virtual ~Window() 
    {
        L_DEBUG("~Window()");
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

    inline void killWindow()
    {
        tagged_delete = true;
    }

    void finalRender();

protected:
    bool open = false;
    bool collapsed = false;
    const char* window_name;
    ImGuiWindowFlags window_flags = 0;
    const bool closeable;
    bool floating_w = false;
    bool floating_clicked = false;
    
private:
    bool tagged_delete = false;
    ImGuiWindowFlags window_flags_last = 0;
    ImVec2 windowSize;
    ImVec2 windowPos;
};
