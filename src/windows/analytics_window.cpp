#include "analytics_window.h"
#include "node_window.h"
#include "../render/nodes/render_node.h"
#include "../render/renderer.h"

static constexpr ImVec4 textColor = ImVec4(0.2f, 0.5f, 0.1f, 1.0f);

void AnalyticsWindow::render()
{
    setWindowPos(ImVec2(0, collapsed_pos_y - ImGui::GetWindowSize().y + 19.0f));
    ImGuiIO& io = ImGui::GetIO();
    

    ImGui::TextColored(textColor, "framerate: %.2f", io.Framerate);
    auto renderNode = nodeWindow->getRenderOutputNode();
    if(renderNode)
    {
        RenderNodeData renderData = nodeWindow->getRenderOutputNode()->data.getValue<RenderNodeData>();
        ImGui::TextColored(textColor, "instances: %u", renderData._instanceCount);
        ImGui::TextColored(textColor, "   motifs: %u",    renderData._motif_span);
        ImGui::TextColored(textColor, "  objects: %u", renderData._instanceCount * renderData._motif_span);
    }
    // Camera should't be nullptr
    Renderer::Camera* camera = nodeWindow->getDrawActiveList()->camera;
    Vector3 cameraPos  = camera->getPosition();
    Vector3 cameraHead = camera->getHeading();
    ImGui::TextColored(textColor, "camera position: (%.1f, %.1f, %.1f)", cameraPos.x, cameraPos.y, cameraPos.z);
    ImGui::TextColored(textColor, "camera  heading: (%.1f, %.1f, %.1f)", cameraHead.x, cameraHead.y, cameraHead.z);
}

void AnalyticsWindow::update()
{
    if(collapsed)
    {
        setWindowPos(ImVec2(0, collapsed_pos_y));
    }
}
