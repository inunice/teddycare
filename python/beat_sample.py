from scipy.signal import find_peaks
import matplotlib.pyplot as plt
import numpy as np
from math import ceil

def get_peaks(beat_pairs, normal = False):
    timestamps = [pair['from_start_device_time'] for pair in beat_pairs]
    values = np.array([pair['ir_value'] for pair in beat_pairs], dtype = 'float64')

    # normalize values
    if normal:
        max_val = values.max()
        values /= max_val
        values *= 255
        values = list(map(ceil, values))

    # TODO: need to fix parameters 
    peaks, _ = find_peaks(values)
    peaks = list(peaks)
    
    return [{'from_start_device_time': timestamps[i], 'ir_value': values[i]} for i in peaks]

    # changepoints = []
    # increasing = beat_pairs_y[0] < beat_pairs_y[1] 
    # for i in range(1, len(beat_pairs)):
    #     if increasing and values[i - 1] > values[i] or not increasing and values[i - 1] < values[i]:
    #         increasing = not increasing
    #         changepoints.append((timestamps[i - 1], values[i - 1]))
    # return changepoints

def get_bpm(beat_pairs):
    num_peaks = len(get_peaks(beat_pairs))
    return num_peaks * 2

if __name__ == "__main__":
    beats = [{'from_start_device_time': pair[0], 'ir_value': pair[1]} for pair in [(1, 300), (2, 400), (3, 500), (4, 350)]]
    '''
    with open("coolterm1.csv") as f:
        for line in f:
            line = list(map(int, line.split(",")))
            beats.append(line)
    '''
    beats_x = [pair['from_start_device_time'] for pair in beats]
    beats_y = [pair['ir_value'] for pair in beats]
    print(beats_x)
    print(beats_y)

    peaks = get_peaks(beats, normal = True)
    peaks_x = [pair['from_start_device_time'] for pair in peaks]
    peaks_y = [pair['ir_value'] for pair in peaks]
    print(peaks_x)
    print(peaks_y)

    plt.plot(beats_x, beats_y)
    plt.scatter(peaks_x, peaks_y)
    plt.title('Heartbeat Data')
    plt.xlabel('Time')
    plt.ylabel('Value')
    plt.show()

