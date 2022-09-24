#include "analytics_window.h"
#include "node_window.h"
#include "../render/nodes/render_node.h"
#include "../render/renderer.h"

static constexpr ImVec4 textColor = ImVec4(0.2f, 0.5f, 0.1f, 1.0f);

void AnalyticsWindow::render()
{
    setWindowPos(ImVec2(optionsWindow->getBarWidth(), collapsed_pos_y - ImGui::GetWindowSize().y + 19.0f));
    ImGuiIO& io = ImGui::GetIO();
    

    ImGui::TextColored(textColor, "framerate: %.2f", io.Framerate);
    auto renderNode = nodeWindow->getRenderOutputNode();
    if(renderNode)
    {
        RenderNodeData renderData = nodeWindow->getRenderOutputNode()->outputs[0]->getValue<RenderNodeData>();
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
    ImGui::TextColored(textColor, " mouse position: (%.1f, %.1f)", io.MousePos.x, io.MousePos.y);
    ImGui::Spacing();
    ImGui::TextColored(textColor, "main viewport position: (%.1f, %.1f)", ImGui::GetMainViewport()->Pos.x, ImGui::GetMainViewport()->Pos.y);

    ImGui::Separator();

    static constexpr int frame_time_size = 128;
    static float frame_times[frame_time_size] = { 0.0f };
    static unsigned long long curr_frame = 0ULL;
    static float last_frame_delta = 0.0f;
    static int max_frame_time_idx = 0;

    frame_times[(int)(curr_frame++ % frame_time_size)] = 1.0f;
    int frame_idx = (int)((curr_frame - 2) % frame_time_size);
    frame_times[frame_idx] = last_frame_delta * 10;
    last_frame_delta = io.DeltaTime;

    ImGui::PlotHistogram("Frametime", frame_times, IM_ARRAYSIZE(frame_times), 0, nullptr, 0.0f, 1.0f, ImVec2(0, 100));
}

void AnalyticsWindow::update()
{
    if(collapsed)
    {
        setWindowPos(ImVec2(optionsWindow->getBarWidth(), collapsed_pos_y));
    }
}
