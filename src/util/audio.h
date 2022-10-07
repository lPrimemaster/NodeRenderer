#pragma once
#include <string>
#include <vector>

namespace Audio
{
    struct AudioInternalData
    {
        int16_t* wav_buffer;
        std::vector<float> peak_energy;
        std::vector<float> rms;
        std::vector<int> zcr;

        int samples_per_frame;
        float ms_per_frame;
        int sample_rate;
    };

    // TODO : Replace all this with waveOutWrite (?)
    void PlayAudio(const std::string& file);
    void PlayAudio(const AudioInternalData* data);
    void StopAudio();

    int GetClosestFrameIndexFromTime(AudioInternalData* aid, float s);

    AudioInternalData LoadMp3FileToMemory(const std::string& filename);
    void DeallocateAudioMemory(AudioInternalData* data);
}
