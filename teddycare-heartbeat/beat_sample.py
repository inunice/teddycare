from scipy.signal import find_peaks
import matplotlib.pyplot as plt
import numpy as np

def get_changepoints(beat):
    timestamps = [x for (x, _) in beat]
    values = np.array([y for (_, y) in beat])

    # normalize values
    max_val = values.max()
    values /= max_val
    
    peaks, _ = find_peaks(values, threshold = 0.7)
    peaks = list(peaks)
    
    return [(timestamps[i], values[i]) for i in sorted(peaks)]

    # changepoints = []
    # increasing = beat_y[0] < beat_y[1] 
    # for i in range(1, len(beat)):
    #     if increasing and beat_y[i - 1] > beat_y[i] or not increasing and beat_y[i - 1] < beat_y[i]:
    #         increasing = not increasing
    #         changepoints.append((beat_x[i - 1], beat_y[i - 1]))
    # return changepoints

def get_bpm(beat):
    num_changepoints = len(get_changepoints(beat))
    return num_changepoints * 2

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

