#include "options_window.h"
#include "node_window.h"

static constexpr ImVec4 textColor = ImVec4(0.2f, 0.5f, 0.1f, 1.0f);

void OptionsWindow::render()
{
    setWindowPos(ImVec2(WIDTH, collapsed_pos_y - ImGui::GetWindowSize().y + 19.0f));
    ImGuiIO& io = ImGui::GetIO();

}

void OptionsWindow::update()
{
    if(collapsed)
    {
        setWindowPos(ImVec2(WIDTH, collapsed_pos_y));
    }
}
