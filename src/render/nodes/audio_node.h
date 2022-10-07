#pragma once
#include "node.h"
#include "../../util/imgui_ext.inl"
#include "../../python/loader.h"
#include "../../util/audio.h"
#include "../../math/comb.inl"
#include <future>
#include <atomic>
#include <chrono>

struct AudioParams
{
    int waveform_sz;
    int beat_times_sz;
    int f_avg_rms_power_sz;
    int f_avg_rms_power_env_sz;
    int f_amps_sz;
    int f_times_sz;
    int f_times_env_sz;
    int freqs_sz;
};

struct AudioNode final : public PropertyNode
{
    struct DisplayMsg
    {
        char data[64];
    };

    inline AudioNode() : PropertyNode(Type::AUDIO, 0, {}, 2, { "power", "envelope" })
    {
        static int inc = 0;
        name = "Audio Node #" + std::to_string(inc++);
        // description =
        //     "This node processes an input audio file (mp3 format only) using python (oof, for now...). "
        //     "It retrives audio power over time and audio power envelope over time. "
        //     "Both outputs consist of a floating point value, describing the audio \"volume\" at a given time. "
        //     "The envelope is similar to a moving average of the power output."
        // ;

        setNamedOutput("power", 0.0f);
        setNamedOutput("envelope", 0.0f);

        setOutputNominalTypes<float>("power", 
            "A floating point value that yields the currently playing audio stream \"volume\" value."
        );
        setOutputNominalTypes<float>("envelope", 
            "A floating point value that yields the currently playing audio stream moving average like \"volume\" value."
        );
    }
    
    ~AudioNode()
    {
        if(playing)
        {
            Audio::StopAudio();
        }

        Audio::DeallocateAudioMemory(&fdata);
        L_TRACE("~AudioNode()");
    }

    inline virtual void update() override
    {
        resetOutputsDataUpdate();

        if(playing)
        {
            // Send a float with current power rms for testing
            float ms = (float)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - audio_start).count();
            int closest_power_rms_idx = Audio::GetClosestFrameIndexFromTime(&fdata, ms);
            // L_TRACE("closest_power_rms_idx: %d", closest_power_rms_idx);
            setNamedOutput("power", fdata.rms[closest_power_rms_idx]);

            // int closest_power_rms_env_idx = Math::FindClosestIdx(fdata.f_times_env, fdata.sizes.f_times_env_sz, s);
            // setNamedOutput("envelope", fdata.f_avg_rms_power_env[closest_power_rms_env_idx]);
        }
    }

    inline virtual void render() override
    {
        // TODO: Make this node a file drag and drop from windows as well
        if(valid)
        {
            ImGui::Text("Currently loaded: %s", std::filesystem::path(to_load).filename().string().c_str());
            ImGui::SameLine();
            if(ImGui::Button(playing ? "Stop" : "Play"))
            {
                if(!playing)
                {
                    Audio::PlayAudio(&fdata);
                    audio_start = std::chrono::steady_clock::now();
                }
                else
                {
                    Audio::StopAudio();
                }
                playing = !playing;
            }
        }
        
        bool closepopup = false;
        static const std::vector<std::string> ext = { ".mp3" };
        if(ImGuiExt::FileBrowser(&to_load, ext))
        {
            if(playing && valid)
            {
                Audio::StopAudio();
            }

            valid = false;
            playing = false;
            // Thread the file loading
            if(!loading)
            {   
                f_fdata = std::async([&]() -> Audio::AudioInternalData {
                    DisplayMsg msg;
                    auto displayMsg = [&](const char* text) -> void { strcpy(msg.data, text); message.store(msg); };
                    displayMsg("Loading audio...\0");
                    Audio::AudioInternalData aid = Audio::LoadMp3FileToMemory(std::filesystem::path(to_load).string());
                    valid = true;
                    return aid;
                });
                loading = true;
            }
        }
        
        if(f_fdata.valid())
        {
            if(f_fdata.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                closepopup = true;
                fdata = f_fdata.get();

                loading = false;

                if(valid)
                {
                    setNamedOutput("power", 0.0f);
                    setNamedOutput("envelope", 0.0f);
                }
                else
                {
                    

                    setNamedOutput("power", EmptyType());
                    setNamedOutput("envelope", EmptyType());
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
            ImGui::Text(message.load().data);
            if(closepopup)
            {
                popupOpened = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    
    inline virtual ByteBuffer serialize() const override
    {
        L_WARNING("Audio Node serialization does not export the audio.");
        ByteBuffer buffer = PropertyNode::serialize();
        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);
    }

private:
    std::string to_load;
    std::future<Audio::AudioInternalData> f_fdata;
    Audio::AudioInternalData fdata;
    bool valid = false;
    bool loading = false;
    bool popupOpened = false;
    bool playing = false;

    std::atomic<DisplayMsg> message;

    std::chrono::steady_clock::time_point audio_start;
};
