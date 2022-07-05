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

struct AudioData
{
    int sample_rate;
    float estimated_tempo;

    float* waveform;
    float* beat_times;
    float* f_avg_rms_power;
    float* f_avg_rms_power_env;
    float* f_amps;
    float* f_times;
    float* f_times_env;
    float* freqs;

    AudioParams sizes;
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

        setNamedOutput("power", 0.0f);
        setNamedOutput("envelope", 0.0f);
    }
    
    ~AudioNode()
    {
        if(audioDataMemory != nullptr)
        {
            delete[] audioDataMemory;
        }
        L_TRACE("~AudioNode()");
    }

    inline virtual void update() override
    {
        resetOutputsDataUpdate();

        if(playing)
        {
            // Send a float with current power rms for testing
            float s = (float)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - audio_start).count() / 1000.0f;
            int closest_power_rms_idx = Math::FindClosestIdx(fdata.f_times, fdata.sizes.f_times_sz, s);
            setNamedOutput("power", fdata.f_avg_rms_power[closest_power_rms_idx]);

            int closest_power_rms_env_idx = Math::FindClosestIdx(fdata.f_times_env, fdata.sizes.f_times_env_sz, s);
            setNamedOutput("envelope", fdata.f_avg_rms_power_env[closest_power_rms_env_idx]);
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
                    Audio::PlayAudio((std::filesystem::path(to_load).stem().string() + ".wav").c_str());
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
                f_fdata = std::async([&]() -> AudioData {
                    DisplayMsg msg;
                    auto displayMsg = [&](const char* text) -> void { strcpy(msg.data, text); message.store(msg); };

                    displayMsg("Heating the oven...\0");

                    auto senv = PythonLoader::StartPythonScriptEnv("beat_analyzer");

                    displayMsg("Converting Audio...\0");

                    auto r0 = PythonLoader::RunPythonScriptFunction<int>(senv, "convert_audio", to_load);

                    displayMsg("Loading Audio...\0");

                    auto r1 = PythonLoader::RunPythonScriptFunction<int>(senv, "load_audio");
                    auto r2 = PythonLoader::RunPythonScriptFunction<AudioParams>(senv, "get_audio_params_size");

                    displayMsg("Allocating Params...\0");

                    // Alocate params size
                    auto sizes = r2.Data;

                    if(audioDataMemory != nullptr)
                    {
                        delete[] audioDataMemory;
                    }
                    int totalSize = sizeof(int) +
                                    sizeof(float) +
                                    sizes.waveform_sz +
                                    sizes.beat_times_sz +
                                    sizes.f_avg_rms_power_sz +
                                    sizes.f_avg_rms_power_env_sz +
                                    sizes.f_amps_sz +
                                    sizes.f_times_sz +
                                    sizes.f_times_env_sz +
                                    sizes.freqs_sz;
                    
                    audioDataMemory = new char[totalSize * sizeof(float)];
                    float* memPtr = (float*)(audioDataMemory + sizeof(int) + sizeof(float));

                    AudioData adata;
                    adata.waveform = memPtr; memPtr += sizes.waveform_sz;
                    adata.beat_times = memPtr; memPtr += sizes.beat_times_sz;
                    adata.f_avg_rms_power = memPtr; memPtr += sizes.f_avg_rms_power_sz;
                    adata.f_avg_rms_power_env = memPtr; memPtr += sizes.f_avg_rms_power_env_sz;
                    adata.f_amps = memPtr; memPtr += sizes.f_amps_sz;
                    adata.f_times = memPtr; memPtr += sizes.f_times_sz;
                    adata.f_times_env = memPtr; memPtr += sizes.f_times_env_sz;
                    adata.freqs = memPtr;
                    adata.sizes = sizes;

                    valid = PythonLoader::RunPythonScriptFunctionCopyReturn(senv, "get_audio_params", audioDataMemory, totalSize * sizeof(float));

                    adata.sample_rate = *(int*)(audioDataMemory);
                    adata.estimated_tempo = *(float*)(audioDataMemory + sizeof(int));

                    displayMsg("Normalizing Power...\0");
                    // float max = 0.0;
                    // for(int i = 0; i < adata.sizes.f_avg_rms_power_sz; i++)
                    // {
                    //     if(adata.f_avg_rms_power[i] > 100.0f) continue;
                    //     if(adata.f_avg_rms_power[i] > max) max = adata.f_avg_rms_power[i];
                    // }

                    // for(int i = 0; i < adata.sizes.f_avg_rms_power_sz; i++)
                    // {
                    //     adata.f_avg_rms_power[i] /= max;
                    // }
                    
                    // max = 0.0;
                    // for(int i = 0; i < adata.sizes.f_avg_rms_power_env_sz; i++)
                    // {
                    //     if(adata.f_avg_rms_power_env[i] > 100.0f) continue;
                    //     if(adata.f_avg_rms_power_env[i] > max) max = adata.f_avg_rms_power_env[i];
                    // }

                    // for(int i = 0; i < adata.sizes.f_times_env_sz; i++)
                    // {
                    //     L_TRACE("%f", adata.f_times_env[i]);
                    //     adata.f_avg_rms_power_env[i] /= max;
                    // }


                    displayMsg("Turning off the gas...\0");

                    PythonLoader::EndPythonScriptEnv(senv);

                    displayMsg("Giving it a spice up...\0");
                    L_DEBUG("Total size used by audio analysis: %dMiB", totalSize * sizeof(float) / 1024 / 1024);

                    return adata;
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
                    fdata.waveform = nullptr;
                    fdata.beat_times = nullptr;
                    fdata.f_avg_rms_power = nullptr;
                    fdata.f_amps = nullptr;
                    fdata.f_times = nullptr;
                    fdata.freqs = nullptr;

                    setNamedOutput("power", fdata);
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

    // TODO
    inline virtual ByteBuffer serialize() const override
    {
        ByteBuffer buffer = PropertyNode::serialize();

        buffer.add(currentmodeid);

        return buffer;
    }

    inline virtual void deserialize(ByteBuffer& buffer) override
    {
        PropertyNode::deserialize(buffer);

        buffer.get(&currentmodeid);
    }

private:
    std::string to_load;
    std::future<AudioData> f_fdata;
    AudioData fdata;
    bool valid = false;
    bool loading = false;
    bool popupOpened = false;
    bool playing = false;

    char* audioDataMemory = nullptr;
    std::atomic<DisplayMsg> message;

    std::chrono::steady_clock::time_point audio_start;
};
