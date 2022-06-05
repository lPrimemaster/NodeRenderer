#pragma once
#include "node.h"
#include "../../util/imgui_ext.inl"
#include "../../python/loader.h"
#include <future>

struct AudioData
{
    int   power_rms_len;
    float power_rms[10240]; // FIXME: Find a way to take this out of the stack
};

struct AudioNode final : public PropertyNode
{
    inline AudioNode() : PropertyNode(AudioData())
    {
        static int inc = 0;
        _input_count = 0;
        _output_count = 1;
        name = "Audio Node #" + std::to_string(inc++);
    }
    
    ~AudioNode()
    {

    }

    inline virtual void render() override
    {
        data.resetDataUpdate();
        // TODO: Make this node a file drag and drop from windows as well

        if(valid)
        {
            ImGui::Text("Currently loaded: %s", std::filesystem::path(to_load).filename().string().c_str());
        }
        
        bool closepopup = false;
        static const std::vector<std::string> ext = { ".mp3" };
        if(ImGuiExt::FileBrowser(&to_load, ext))
        {
            // Thread the file loading
            if(!loading)
            {
                f_fdata = std::async(PythonLoader::RunPythonScriptMain<AudioData, std::string>, "beat_analyzer", to_load);
                loading = true;
            }
        }
        
        if(f_fdata.valid())
        {
            if(f_fdata.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                closepopup = true;
                PythonLoader::PythonReturn<AudioData> ldata = f_fdata.get();
                valid = ldata.Valid;
                fdata = ldata.Data;
                loading = false;
                
                if(valid)
                {
                    data.setValue(fdata);
                }
            }
            else if(loading && !popupOpened)
            {
                popupOpened = true;
                L_TRACE("Loading popup open.");
                ImGui::OpenPopup("Please Wait");
            }
        }

        if(ImGui::BeginPopupModal("Please Wait", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGuiExt::SpinnerText();
            ImGui::SameLine();
            ImGui::Text("Loading File...");
            if(closepopup)
            {
                popupOpened = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

private:
    std::string to_load;
    std::future<PythonLoader::PythonReturn<AudioData>> f_fdata;
    AudioData fdata;
    bool valid = false;
    bool loading = false;
    bool popupOpened = false;
};
