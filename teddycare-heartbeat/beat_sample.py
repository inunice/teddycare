from scipy.signal import find_peaks
import matplotlib.pyplot as plt
import numpy as np

def get_peaks(beat_pairs):
    timestamps = [pair['from_start_device_time'] for pair in beat_pairs]
    values = np.array([pair['ir_value'] for pair in beat_pairs], dtype = 'float64')

    # normalize values
    max_val = values.max()
    values /= max_val
    
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
    test = []
    with open("coolterm1.csv") as f:
        for line in f:
            line = list(map(int, line.split(",")))
            test.append(line)
    test_x = [x for (x, _) in test]
    test_y = [y for (_, y) in test]

    changepoints = get_changepoints(test)
    change_x = [x for (x, _) in changepoints]
    change_y = [y for (_, y) in changepoints]

    '''
    bpm = get_bpm(test)
    print(bpm)
    '''

    plt.plot(test_x, test_y)
    plt.scatter(change_x, change_y)
    plt.title('Sample Data')
    plt.xlabel('Time')
    plt.ylabel('Value')
    plt.show()

