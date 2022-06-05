#pragma once
#include <vector>
#include "../../imgui/imgui.h"
#include "window.inl"
#include "../render/nodes/node.h"

namespace Renderer
{
    struct DrawList;
}

class NodeWindow;

class AnalyticsWindow : public Window
{
public:
    AnalyticsWindow(const char* name, NodeWindow* nodeWindow) : Window(name, false), nodeWindow(nodeWindow)
    {
        open = true; 
        window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    }

    ~AnalyticsWindow() {  }

    inline void setCollapsedPosY(float y)
    {
        collapsed_pos_y = y;
    }

    virtual void render() override;
    virtual void update() override;

    inline static constexpr float WIDTH = 350.0f;

private:
    NodeWindow* nodeWindow;
    float collapsed_pos_y = 0.0f;
};
