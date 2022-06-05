import librosa as lr
import librosa.display as lrd
import time
import matplotlib.pyplot as plt
import numpy as np
import math
import struct
import ctypes
from ctypes import c_long, c_wchar_p, c_ulong, c_void_p
gHandle = ctypes.windll.kernel32.GetStdHandle(c_long(-11))

def find_nearest_idx(array, value, N=1):
    return np.argpartition(np.abs(array - value), N)[:N]

def interpolate(RMS, times, t):
    nt = find_nearest_idx(times, t, 1)
    return RMS[nt]

def move (y, x):
   """Move cursor to position indicated by x and y."""
   value = x + (y << 16)
   ctypes.windll.kernel32.SetConsoleCursorPosition(gHandle, c_ulong(value))


def main(song_name):
    song_name = str(song_name, 'utf-8')
    y, sr = lr.load(f'{song_name}')
    tempo, beat_frames = lr.beat.beat_track(y=y, sr=sr)
    beat_times = lr.frames_to_time(beat_frames, sr=sr)
    freq = lr.fft_frequencies()
    print(freq)
    S, _ = lr.magphase(lr.stft(y))
    rms = lr.feature.rms(S=S)
    times = lr.times_like(rms)

    # _, ax = plt.subplots(nrows=1, sharex=True)
    # print(rms[0])
    # ax.plot(times, rms[0], label='RMS Energy')
    # ax.set(xticks=[])
    # ax.legend()
    # ax.label_outer()

    # plt.show()

    # ti = time.time()
    # t = 0

    # for t in [3, 2, 1]:
    #     print(t)
    #     time.sleep(1)
    # print('Go!')
    # print('\n' * 100)

    # mr = 100
    # ra = []
    # rt = []
    # lt = ti - 1
    # minrms = 50
    # maxrms = 0

    # while t < 200:
    #     t = time.time() - ti
    #     r = int(interpolate(rms[0], times, t)[0] * 100)
    #     ft = t - lt
    #     ra.append(r)
    #     rt.append(ft)
    #     lt = t

    #     if sum(rt[1:]) > 3:
    #         ra.pop(0)
    #         rt.pop(0)

    #     lra = len(ra)
    #     if lra <= 0:
    #         lra = 1
        
    #     avgrms = sum(ra) / lra

    #     c = '*' * r
    #     w = ' ' * (100 - r)

    #     move(0, 0)
    #     print(f'Power RMS : {c}{w}')

    #     r1 = int(interpolate(S[0][:], times, t)[0] * 0.1)
    #     c1 = '*' * r1
    #     w1 = ' ' * (70 - r1)
    #     move(1, 0)
    #     print(f'~{freq[0]:.1f}Hz RMS : {c1}{w1}')

    #     r1 = int(interpolate(S[1][:], times, t)[0] * 0.1)
    #     c1 = '*' * r1
    #     w1 = ' ' * (70 - r1)
    #     move(2, 0)
    #     print(f'~{freq[1]:.1f}Hz RMS : {c1}{w1}')

    #     r1 = int(interpolate(S[2][:], times, t)[0] * 0.1)
    #     c1 = '*' * r1
    #     w1 = ' ' * (70 - r1)
    #     move(3, 0)
    #     print(f'~{freq[2]:.1f}Hz RMS : {c1}{w1}')

    #     r1 = int(interpolate(S[3][:], times, t)[0] * 0.1)
    #     c1 = '*' * r1
    #     w1 = ' ' * (70 - r1)
    #     move(4, 0)
    #     print(f'~{freq[3]:.1f}Hz RMS : {c1}{w1}')

    #     r1 = int(interpolate(S[4][:], times, t)[0] * 0.1)
    #     c1 = '*' * r1
    #     w1 = ' ' * (70 - r1)
    #     move(5, 0)
    #     print(f'~{freq[4]:.1f}Hz RMS : {c1}{w1}')

    #     move(6, 0)
    #     print(f'Power Rolling Avg RMS     ( 3s) : {avgrms:.2f}')

    #     if minrms > avgrms: minrms = avgrms
    #     move(7, 0)
    #     print(f'Power Rolling Min RMS     ( 3s) : {minrms:.2f}')

    #     if maxrms < avgrms: maxrms = avgrms
    #     move(8, 0)
    #     print(f'Power Rolling Max RMS     ( 3s) : {maxrms:.2f}')

    #     move(9, 0)
    #     fpower = 2**avgrms
    #     print(f'F-Power Rolling Avg RMS   ( 3s) : {fpower:.2f}')

    #     move(10, 0)
    #     print(f'Frame time : {ft:.5f}')

    return struct.pack(f'<i{len(rms[0])}f', len(rms[0]), *(rms[0]))
