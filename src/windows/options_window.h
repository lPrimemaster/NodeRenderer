#pragma once
#include <vector>
#include "../../imgui/imgui.h"
#include "window.inl"
#include "../render/nodes/node.h"
#include <unordered_map>
#include "../../res/image/res.h"

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
        setWindowCollapsed(false);
        window_flags = 
            ImGuiWindowFlags_NoMove          |
            ImGuiWindowFlags_NoResize        |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDecoration    |
            ImGuiWindowFlags_NoCollapse      |
            ImGuiWindowFlags_NoTitleBar    /*|
            ImGuiWindowFlags_NoBackground */;

            loadIcons({ IDI_ICON_NEWFILE, IDI_ICON_LOADFILE, IDI_ICON_SAVEFILE });
    }

    ~OptionsWindow() 
    { 
        if(b64buffer) delete[] b64buffer;

        for(auto i : icons_id)
        {
            glDeleteTextures(1, &i.second);
        }
    }

    inline void setCollapsedPosY(float y)
    {
        collapsed_pos_y = y;
    }

    virtual void render() override;
    virtual void update() override;

    inline static constexpr float COLLAPSED_WIDTH = 20.0f;
    inline static constexpr float OPENED_WIDTH = 200.0f;

    void loadIcons(const std::initializer_list<const int> resources_id);

    inline const float getBarWidth() const
    {
        return bar_open ? OPENED_WIDTH : COLLAPSED_WIDTH;
    }

private:
    NodeWindow* nodeWindow;
    float collapsed_pos_y = 0.0f;
    float bar_width = 10.0f;

    std::unordered_map<int, GLuint> icons_id;

    bool bar_open = false;

    std::string data64;
    char* b64buffer = nullptr;
};
