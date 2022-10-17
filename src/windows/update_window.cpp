#include "update_window.h"
#include "version.h"
#include "../util/imgui_ext.inl"

void UpdateCheckWindow::render()
{
    // Center the window on the screen
    ImGuiIO& io = ImGui::GetIO();
    setWindowSize(window_size);
    setWindowPos(ImVec2(io.DisplaySize.x / 2, io.DisplaySize.y / 2) - ImVec2(window_size.x / 2, window_size.y / 2));

    if(!popup_open) 
    { 
        ImGui::OpenPopup("Update Available");
        popup_open = true;
    }

    if(ImGui::BeginPopupModal("Update Available", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
    {
        if(!updating)
        {
            ImGui::Text("Found new version: %s", udata.v_string.c_str());
            ImGui::Text("Current version: %s", NodeRenderer_VERSION_FULL);

            ImGui::NewLine();
            ImGui::Text("Do you wish to update?");
            
            if(ImGui::Button("Yes"))
            {
                dl_future = std::async(std::launch::async, [&]() -> void {
                    DownloadClient(udata.download_url, &current_download_size);
                });
                updating = true;
            }
            ImGui::SameLine();
            if(ImGui::Button("Later"))
            {
                ImGui::CloseCurrentPopup();
            }
        }
        else
        {
            ImGui::Text("Downloading new installer...");

            size_t current_B = current_download_size.load();

            float total_kB = (float)(udata.download_size / 1024) + (float)((udata.download_size % 1024) / 1024);
            float current_kB = (float)(current_B / 1024) + (float)((current_B % 1024) / 1024);
            float pct = current_kB / total_kB;
            ImGuiExt::ProgressBar(pct, ImGui::GetWindowWidth());
            ImGui::SameLine();
            ImGui::Text("%.1f%%", 100 * pct);
            ImGui::NewLine();

            if(dl_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                if(ImGui::Button("Close and Update."))
                {
                    if(UpdateClient())
                    {
                        // Terminate current process cleanly
                        glfwSetWindowShouldClose(glfw_window_handle, true);
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
        }

        ImGui::EndPopup();
    }
}

void UpdateCheckWindow::update()
{
    if(!new_update)
    {
        killWindow();
    }
}
