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

class OptionsWindow : public Window
{
public:
    OptionsWindow(const char* name, NodeWindow* nodeWindow) : Window(name, false), nodeWindow(nodeWindow)
    {
        open = true;
        setWindowCollapsed(true);
        window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
    }

    ~OptionsWindow() 
    { 
        if(b64buffer) delete[] b64buffer; 
    }

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

    std::string data64;
    char* b64buffer = nullptr;
};
