#include "audio.h"
#ifdef _WIN32
#include <Windows.h>
#elif
#error "Audio playback is only implemented for windows."
#endif

#include "../log/logger.h"

void Audio::PlayAudio(const std::string& file)
{
    // TODO: Change to SND_MEMORY (might be faster to start playing - research about this)
    PlaySound(std::string("res/sound/" + file).c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

void Audio::StopAudio()
{
    PlaySound(NULL, 0, 0);
}
