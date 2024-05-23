from scipy.signal import find_peaks
import matplotlib.pyplot as plt


def get_changepoints(beat):
    beat_x = [x for (x, _) in beat]
    beat_y = [y for (_, y) in beat]
    n_beat_y = [-y for y in beat_y]
    
    peaks, _ = find_peaks(beat_y, threshold = 100)
    troughs, _ = find_peaks(n_beat_y, threshold = 100)
    peaks, troughs = list(peaks), list(troughs)
    
    return [(beat_x[i], beat_y[i]) for i in sorted(peaks + troughs)]

    # changepoints = []
    # increasing = beat_y[0] < beat_y[1] 
    # for i in range(1, len(beat)):
    #     if increasing and beat_y[i - 1] > beat_y[i] or not increasing and beat_y[i - 1] < beat_y[i]:
    #         increasing = not increasing
    #         changepoints.append((beat_x[i - 1], beat_y[i - 1]))
    # return changepoints

def get_bpm(beat):
    slice = beat[50: 50 + 60]
    num_changepoints = len(get_changepoints(slice))
    return num_changepoints / 60

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

