import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from scipy.signal import find_peaks
from math import ceil

def normalize(y, old_min, old_max, new_min = 100, new_max = 200):
    a = (new_max - new_min) / (old_max - old_min)
    b = new_max - a * old_max
    return min(max(ceil(a * y + b), new_min), 255)

def get_troughs(beat_pairs):
    timestamps = [pair['from_start_device_time'] for pair in beat_pairs]
    values = [pair['ir_value'] for pair in beat_pairs]
    neg_values = [-value for value in values]

    # TODO: need to fix parameters 
    troughs, _ = find_peaks(neg_values, width = 2)
    return [{'from_start_device_time': timestamps[i], 'ir_value': values[i], 'peak': 0} for i in troughs]

def get_peaks(beat_pairs):
    timestamps = [pair['from_start_device_time'] for pair in beat_pairs]
    values = [pair['ir_value'] for pair in beat_pairs]

    # TODO: need to fix parameters 
    peaks, _ = find_peaks(values, distance = 8)
    return [{'from_start_device_time': timestamps[i], 'ir_value': values[i], 'peak': 1} for i in peaks]

def calculate_delays(beat_pairs):
    peaks = get_peaks(beat_pairs)
    troughs = get_troughs(beat_pairs)
    spikes = peaks + troughs
    spikes.sort(key = lambda tup: tup['from_start_device_time'])

    #calculate delays
    for i in range(1, len(spikes)):
        spikes[i]['delay'] = spikes[i]['from_start_device_time'] - spikes[i - 1]['from_start_device_time']
    spikes[0]['delay'] = spikes[-1]['delay']

    # remove extra attribute
    for spike in spikes:
        del spike['from_start_device_time']

    # convert values to pwm
    max_val = max(spike['ir_value'] for spike in spikes)
    min_val = min(spike['ir_value'] for spike in spikes)
    for spike in spikes:
        spike['ir_value'] = normalize(spike['ir_value'], min_val, max_val)

    return spikes

# call this function whenever the is_recording value changes
def preprocess():
    # As an admin, the app has access to read and write all data, regradless of Security Rules
    ref = db.reference('heartbeat_data')
    rec_ref = ref.child('recordings')
    is_recording = ref.child('is_recording').get()
    recordings = rec_ref.get()

    if is_recording == '0' and recordings != None and len(recordings) > 0:
        recording_list = []
        for recording in recordings.values():
            recording_list.extend(recording[:-1])
        # preprocess
        ref.update({
            'preprocessed_heartbeat': {
                'spikes': calculate_delays(recording_list)
                }
            })
        # remove recordings
        # ref.update({'recordings': None})
        print('processed')

cred = credentials.Certificate('test1-a4e94-firebase-adminsdk-mv0uj-28a74beeb8.json')

# Initialize the app with a service account, granting admin privileges
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://test1-a4e94-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })

# test
def test():
    import matplotlib.pyplot as plt

    ref = db.reference('heartbeat_data')
    rec_ref = ref.child('recordings')
    recordings = rec_ref.get()
    recording_list = []
    for recording in recordings.values():
        recording_list.extend(recording[:-1])

    beats = recording_list

    # delays = [beats[i]['from_start_device_time'] - beats[i - 1]['from_start_device_time'] for i in range(1, len(beats))]
    # print(f'average delay is {sum(delays) / len(delays)}')

    beats_x = [pair['from_start_device_time'] for pair in beats]
    beats_y = [pair['ir_value'] for pair in beats]

    peaks = get_peaks(beats)
    peaks_x = [pair['from_start_device_time'] for pair in peaks]
    peaks_y = [pair['ir_value'] for pair in peaks]

    troughs = get_troughs(beats)
    troughs_x = [pair['from_start_device_time'] for pair in troughs]
    troughs_y = [pair['ir_value'] for pair in troughs]

    plt.plot(beats_x, beats_y)
    plt.scatter(peaks_x, peaks_y, c = 'green')
    plt.scatter(troughs_x, troughs_y, c = 'red')
    plt.title('Heartbeat Data')
    plt.xlabel('Time')
    plt.ylabel('Value')
    plt.show()

    # preprocess()

