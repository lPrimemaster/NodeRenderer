#pragma once
#include <string>

namespace Audio
{
    // TODO : Replace all this with waveOutWrite (?)
    void PlayAudio(const std::string& file);
    void StopAudio();
}
