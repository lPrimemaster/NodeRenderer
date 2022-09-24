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
class OptionsWindow;

class AnalyticsWindow : public Window
{
public:
    AnalyticsWindow(const char* name, NodeWindow* nodeWindow, OptionsWindow* optionsWindow) : Window(name, false), nodeWindow(nodeWindow), optionsWindow(optionsWindow)
    {
        open = true; 
        setWindowCollapsed(false);
        window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings /* | ImGuiWindowFlags_NoBackground */;
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
    OptionsWindow* optionsWindow;
    float collapsed_pos_y = 0.0f;
};
