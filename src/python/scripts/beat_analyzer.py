import librosa as lr
import numpy as np
import struct
import subprocess

def find_nearest_idx(array, value, N=1):
    return np.argpartition(np.abs(array - value), N)[:N]

def interpolate(RMS, times, t):
    nt = find_nearest_idx(times, t, 1)
    return RMS[nt]


class AudioAttrs:
    waveform = None
    sample_rate = None
    estimated_tempo = None
    beat_times = None

    f_avg_rms_power = None
    f_amps = None
    f_times = None

    freqs = None

    sizes = []

g_audio_attrs = AudioAttrs()
file = ''

def convert_audio(song_name):
    # Check if audio is already there (NOTE: this won't work for files with the same name)
    song_name = str(song_name, 'utf-8')
    name = song_name.split('\\')[-1].split('.')[0]
    global file
    file = f'res/sound/{name}.wav'
    subprocess.call(['ffmpeg', '-y', '-i', song_name, file], stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)

    # Assuming nothing fails
    return struct.pack(f'<i', 0)

def load_audio():
    global g_audio_attrs

    y, sr = lr.load(file)
    tempo, beat_frames = lr.beat.beat_track(y=y, sr=sr)
    beat_times = lr.frames_to_time(beat_frames, sr=sr)
    freq = lr.fft_frequencies(sr=sr)
    S, _ = lr.magphase(lr.stft(y))
    rms = lr.feature.rms(S=S)
    times = lr.times_like(rms)

    g_audio_attrs.waveform = y
    g_audio_attrs.sample_rate = sr
    g_audio_attrs.estimated_tempo = tempo
    g_audio_attrs.beat_times = beat_times
    g_audio_attrs.f_avg_rms_power = rms[0]
    g_audio_attrs.f_amps = S
    g_audio_attrs.f_times = times
    g_audio_attrs.freqs = freq

    # Assuming nothing fails
    return struct.pack(f'<i', 0)

def get_audio_params_size():
    global g_audio_attrs

    g_audio_attrs.sizes.append(len(g_audio_attrs.waveform))
    g_audio_attrs.sizes.append(len(g_audio_attrs.beat_times))
    g_audio_attrs.sizes.append(len(g_audio_attrs.f_avg_rms_power))
    g_audio_attrs.sizes.append(g_audio_attrs.f_amps.size)
    g_audio_attrs.sizes.append(len(g_audio_attrs.f_times))
    g_audio_attrs.sizes.append(len(g_audio_attrs.freqs))

    return struct.pack(
        '<iiiiii',
        g_audio_attrs.sizes[0],
        g_audio_attrs.sizes[1],
        g_audio_attrs.sizes[2],
        g_audio_attrs.sizes[3],
        g_audio_attrs.sizes[4],
        g_audio_attrs.sizes[5]
    )

def get_audio_params():
    global g_audio_attrs
    c = g_audio_attrs.sizes
    return struct.pack(
        f'<{c[0]}fif{c[1]}f{c[2]}f{c[3]}f{c[4]}f{c[5]}f',
        *g_audio_attrs.waveform,
        g_audio_attrs.sample_rate,
        g_audio_attrs.estimated_tempo,
        *g_audio_attrs.beat_times,
        *g_audio_attrs.f_avg_rms_power,
        *g_audio_attrs.f_amps.flatten(),
        *g_audio_attrs.f_times,
        *g_audio_attrs.freqs
    )

import matplotlib.pyplot as plt
from scipy.interpolate import interp1d

def moving_average(a, n=3) :
    ret = np.cumsum(a, dtype=float)
    ret[n:] = ret[n:] - ret[:-n]
    return ret[n - 1:] / n

def hl_envelopes_idx(s, dmin=1, dmax=1, split=False):
    """
    Input :
    s: 1d-array, data signal from which to extract high and low envelopes
    dmin, dmax: int, optional, size of chunks, use this if the size of the input signal is too big
    split: bool, optional, if True, split the signal in half along its mean, might help to generate the envelope in some cases
    Output :
    lmin,lmax : high/low envelope idx of input signal s
    """

    # locals min      
    lmin = (np.diff(np.sign(np.diff(s))) > 0).nonzero()[0] + 1 
    # locals max
    lmax = (np.diff(np.sign(np.diff(s))) < 0).nonzero()[0] + 1 
    

    if split:
        # s_mid is zero if s centered around x-axis or more generally mean of signal
        s_mid = np.mean(s) 
        # pre-sorting of locals min based on relative position with respect to s_mid 
        lmin = lmin[s[lmin]<s_mid]
        # pre-sorting of local max based on relative position with respect to s_mid 
        lmax = lmax[s[lmax]>s_mid]


    # global max of dmax-chunks of locals max 
    lmin = lmin[[i+np.argmin(s[lmin[i:i+dmin]]) for i in range(0,len(lmin),dmin)]]
    # global min of dmin-chunks of locals min 
    lmax = lmax[[i+np.argmax(s[lmax[i:i+dmax]]) for i in range(0,len(lmax),dmax)]]
    
    return lmin,lmax

def run_manually_test():
    global file
    file = f'res/sound/scatman.wav'
    load_audio()
    figure, axis = plt.subplots(2, 1)
    normalized = g_audio_attrs.f_avg_rms_power / np.max(g_audio_attrs.f_avg_rms_power)
    t = np.linspace(0, 200, num=len(normalized), endpoint=True)
    _, hi = hl_envelopes_idx(normalized, 2, 10)

    axis[0].plot(t, normalized)
    axis[0].plot(t[hi], normalized[hi])
    # f = interp1d(t[hi], normalized[hi], kind='cubic')
    # ny = np.array([f(i) if i > np.min(t) and i < np.max(t) else 0.0 for i in t])
    # axis[0].plot(t, ny)
    # axis[0].plot(t[hi][:-4]+5, moving_average(normalized[hi], n=5))
    # axis[1].plot(moving_average(g_audio_attrs.f_avg_rms_power, n=21) / np.max(moving_average(g_audio_attrs.f_avg_rms_power, n=21)))
    # axis[1].plot(moving_average(g_audio_attrs.f_avg_rms_power, n=51) / np.max(moving_average(g_audio_attrs.f_avg_rms_power, n=51)))
    # axis[1].plot(moving_average(g_audio_attrs.f_avg_rms_power, n=101) / np.max(moving_average(g_audio_attrs.f_avg_rms_power, n=101)))
    # axis[0].plot(moving_average(g_audio_attrs.f_avg_rms_power, n=201) / np.max(moving_average(g_audio_attrs.f_avg_rms_power, n=201)))
    plt.show()


run_manually_test()