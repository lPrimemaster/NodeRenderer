#include "audio.h"
#ifdef _WIN32
#include <Windows.h>
#elif
#error "Audio playback is only implemented for windows."
#endif

#define MINIMP3_IMPLEMENTATION
#include "../../minimp3/minimp3_ex.h"
#include "../../minimp3/minimp3.h"

#include "../log/logger.h"

#include <cassert>

#include "../../kissfft/kiss_fft.h"

#define PCM16_MIN_SIZE (int16_t)0x8000
#define PCM16_MAX_SIZE (int16_t)0x7FFF
#define PI_F 3.141592654f

struct frames_iterate_data
{
    mp3dec_t *mp3d;
    mp3dec_file_info_t *info;
    Audio::AudioInternalData* aid;
    size_t allocated = 0;
    bool first_frame = true;

    kiss_fft_cfg fft_cfg;
    kiss_fft_cpx* fft_in;
    kiss_fft_cpx* fft_out;
    float* fft_window = nullptr;
    float* fft_mag_spectrum = nullptr;
};

static const float s16_to_f32_unity(const int16_t value)
{
    return (float)value / (float)PCM16_MAX_SIZE;
}

static float* create_hamming_window(const int samples)
{
    float* window = new float[samples];

    float samples_minus_one = samples - 1;
    constexpr float two_pi = 2.0f * PI_F;

    for(int i = 0; i < samples; i++)
    {
        window[i] = 0.54f - (0.46f * cosf(two_pi * i / samples_minus_one));
    }

    return window;
}

static void calculate_frame_fft(frames_iterate_data* d, const int16_t* buffer, const int samples)
{
    for(int i = 0; i < samples; i++)
    {
        d->fft_in[i].r = s16_to_f32_unity(buffer[i]) * d->fft_window[i];
        d->fft_in[i].i = 0.0f;
    }

    kiss_fft(d->fft_cfg, d->fft_in, d->fft_out);
}

static void calculate_frame_magnitude_spectrum(frames_iterate_data* d, const int samples)
{
    for(int i = 0; i < samples / 2; i++)
    {
        d->fft_mag_spectrum[i] = sqrtf(d->fft_out[i].r * d->fft_out[i].r + d->fft_out[i].i * d->fft_out[i].i);
    }
}

static float calculate_frame_high_freq_sum(const float* mag_spectrum, const int samples)
{
    float sum = 0.0f;
    
    for(int i = 0; i < samples / 2; i++)
    {
        sum += mag_spectrum[i] * (float)(i + 1);
    }

    return sum;
}

static float calculate_frame_low_freq_sum(const float* mag_spectrum, const int samples)
{
    float sum = 0.0f;
    
    for(int i = 0; i < samples / 2; i++)
    {
        sum += mag_spectrum[i] * (float)((samples / 2) - i);
    }

    return sum;
}

static float calculate_frame_peak_energy(const int16_t* buffer, const int samples)
{
    int16_t peak = PCM16_MIN_SIZE;

    for(int i = 0; i < samples; i++)
    {
        int16_t sample_abs = abs(buffer[i]);

        if(sample_abs > peak)
        {
            peak = sample_abs;
        }
    }

    return s16_to_f32_unity(peak);
}

static float calculate_frame_rms(const int16_t* buffer, const int samples)
{
    float sum = 0.0f;

    for(int i = 0; i < samples; i++)
    {
        sum += powf(s16_to_f32_unity(buffer[i]), 2.0f);
    }

    return sqrtf(sum / samples);
}

static int calculate_frame_zcr(const int16_t* buffer, const int samples)
{
    int zcr = 0;

    for(int i = 1; i < samples; i++)
    {
        bool current = (buffer[i] > 0);
        bool previous = (buffer[i-1] > 0);

        if(current != previous)
        {
            zcr++;
        }
    }

    return zcr;
}

// From https://docs.fileformat.com/audio/wav/
static char* create_wav_header(int hz, int ch, int bips, int data_bytes)
{
    static char hdr[44];
    unsigned long nAvgBytesPerSec = bips*ch*hz >> 3;
    unsigned int nBlockAlign      = bips*ch >> 3;
    *( int8_t *)(hdr + 0x00) = 'R';
    *( int8_t *)(hdr + 0x01) = 'I';
    *( int8_t *)(hdr + 0x02) = 'F';
    *( int8_t *)(hdr + 0x03) = 'F';
    *(int32_t *)(hdr + 0x04) = 44 + data_bytes - 8;
    *( int8_t *)(hdr + 0x08) = 'W';
    *( int8_t *)(hdr + 0x09) = 'A';
    *( int8_t *)(hdr + 0x0A) = 'V';
    *( int8_t *)(hdr + 0x0B) = 'E';
    *( int8_t *)(hdr + 0x0C) = 'f';
    *( int8_t *)(hdr + 0x0D) = 'm';
    *( int8_t *)(hdr + 0x0E) = 't';
    *( int8_t *)(hdr + 0x0F) = ' ';
    *(int32_t *)(hdr + 0x10) = 16;
    *(int16_t *)(hdr + 0x14) = 1;
    *(int16_t *)(hdr + 0x16) = ch;
    *(int32_t *)(hdr + 0x18) = hz;
    *(int32_t *)(hdr + 0x1C) = nAvgBytesPerSec;
    *(int16_t *)(hdr + 0x20) = nBlockAlign;
    *(int16_t *)(hdr + 0x22) = bips;
    *( int8_t *)(hdr + 0x24) = 'd';
    *( int8_t *)(hdr + 0x25) = 'a';
    *( int8_t *)(hdr + 0x26) = 't';
    *( int8_t *)(hdr + 0x27) = 'a';
    *(int32_t *)(hdr + 0x28) = data_bytes;
    return hdr;
}

// Decode the frames and calculate some fancy stuff for displaying like rms/power/specfreq/...
static int frames_iterate_cb(void *user_data, const uint8_t *frame, int frame_size, int free_format_bytes, size_t buf_size, uint64_t offset, mp3dec_frame_info_t *info)
{
    (void)buf_size;
    (void)offset;
    (void)free_format_bytes;
    frames_iterate_data* d = (frames_iterate_data*)user_data;
    d->info->channels = info->channels;
    d->info->hz       = info->hz;
    d->info->layer    = info->layer;
    /*printf("%d %d %d\n", frame_size, (int)offset, info->channels);*/
    if ((d->allocated - d->info->samples*sizeof(mp3d_sample_t)) < MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t))
    {
        if (!d->allocated)
            d->allocated = 1024*1024;
        else
            d->allocated *= 2;
        mp3d_sample_t* alloc_buf = (mp3d_sample_t*)realloc(d->info->buffer, d->allocated);
        if (!alloc_buf)
            return MP3D_E_MEMORY;
        d->info->buffer = alloc_buf;
    }
    int samples = mp3dec_decode_frame(d->mp3d, frame, frame_size, d->info->buffer + d->info->samples, info);
    if (samples)
    {
        const int16_t* buffer = d->info->buffer + d->info->samples;
        int total_samples = samples*info->channels;
        d->info->samples += total_samples;

        // Time domain calculations
        float frame_peak_energy = calculate_frame_peak_energy(buffer, total_samples);
        float frame_rms         = calculate_frame_rms(buffer, total_samples);
        int   frame_zcr         = calculate_frame_zcr(buffer, total_samples);

        d->aid->peak_energy.push_back(frame_peak_energy);
        d->aid->rms.push_back(frame_rms);
        d->aid->zcr.push_back(frame_zcr);

        if(d->first_frame)
        {
            d->first_frame = false;
            d->aid->samples_per_frame = samples;

            d->fft_cfg = kiss_fft_alloc(samples, 0, 0, 0);
            d->fft_in = new kiss_fft_cpx[samples];
            d->fft_out = new kiss_fft_cpx[samples];
            d->fft_mag_spectrum = new float[samples/2];
            d->fft_window = create_hamming_window(samples);
        }

        calculate_frame_fft(d, buffer, samples);
        calculate_frame_magnitude_spectrum(d, samples);

        float frame_high_freq = calculate_frame_high_freq_sum(d->fft_mag_spectrum, samples);
        float frame_low_freq  = calculate_frame_low_freq_sum(d->fft_mag_spectrum, samples);

        d->aid->high_freq_sum.push_back(frame_high_freq);
        d->aid->low_freq_sum.push_back(frame_low_freq);
    }
    return 0;
}

void Audio::PlayAudio(const std::string& file)
{
    PlaySound(std::string("res/sound/" + file).c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

void Audio::PlayAudio(const AudioInternalData* data)
{
    if(data->wav_buffer)
    {
        if(!PlaySound((LPCSTR)data->wav_buffer, NULL, SND_MEMORY | SND_ASYNC))
        {
            L_ERROR("PlaySound Error...");
        }
    }
    else
    {
        L_ERROR("Attemped to play audio data from an invalid WAV buffer.");
    }
}

void Audio::StopAudio()
{
    PlaySound(NULL, 0, 0);
}

int Audio::GetClosestFrameIndexFromTime(AudioInternalData* aid, float s)
{
    int idx = (int)(s / aid->ms_per_frame);
    if(idx >= aid->rms.size()) return 0;
    return idx;
}

Audio::AudioInternalData Audio::LoadMp3FileToMemory(const std::string& filename)
{
    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    memset(&info, 0, sizeof(info));
    Audio::AudioInternalData aid;

    frames_iterate_data d;
    d.mp3d = &mp3d;
    d.info = &info;
    d.aid = &aid;

    mp3dec_init(&mp3d);
    int res = mp3dec_iterate(filename.c_str(), frames_iterate_cb, &d);

    // Did we get the correct number of frames ?
    // assert(aid.rms.size() == info.samples / info->channels / ); ?????

    // Sound wave sample data is inside info
    aid.sample_rate = info.hz;
    aid.ms_per_frame = ((float)aid.samples_per_frame / (float)info.hz) * 1000.0f;

    size_t pcm_data_size = info.samples * sizeof(int16_t);
    size_t buffer_size = pcm_data_size + 44;

    aid.wav_buffer = new int16_t[buffer_size];
    int16_t* pcm_data = (int16_t*)(((char*)aid.wav_buffer) + 44);

    // Copy audio data
    memcpy(pcm_data, info.buffer, pcm_data_size);

    // Copy riff wave header
    memcpy(aid.wav_buffer, create_wav_header(info.hz, info.channels, 16, pcm_data_size), 44);

    FILE* f = fopen("objdumps/wave_conv.wav", "wb");

    fwrite(aid.wav_buffer, 1, buffer_size, f);

    fclose(f);

    free(d.fft_cfg);
    delete[] d.fft_in;
    delete[] d.fft_out;
    delete[] d.fft_window;
    delete[] d.fft_mag_spectrum;

    free(info.buffer);

    L_DEBUG("MP3 loading OK.");

    return aid;
}

void Audio::DeallocateAudioMemory(AudioInternalData* data)
{
    if(data->wav_buffer)
    {
        delete[] data->wav_buffer;
        data->peak_energy.clear();
        data->rms.clear();
        data->zcr.clear();
        L_DEBUG("Audio deallocation OK.");
    }
    else
    {
        L_ERROR("Attempted to deallocate and invalid internal audio buffer.");
    }
}
