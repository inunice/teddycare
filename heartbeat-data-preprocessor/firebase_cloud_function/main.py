from typing import Any

# The Cloud Functions for Firebase SDK to create Cloud Functions and set up triggers.
from firebase_functions import db_fn, https_fn

# The Firebase Admin SDK to access the Firebase Realtime Database.
from firebase_admin import initialize_app, db

from scipy.signal import find_peaks
from math import ceil

# Helper functions
def normalize(y, old_min, old_max, new_min = 100, new_max = 235):
    a = (new_max - new_min) / (old_max - old_min)
    b = new_max - a * old_max
    return min(max(ceil(a * y + b), new_min), new_max)

def get_troughs(beat_pairs):
    timestamps = [pair['from_start_device_time'] for pair in beat_pairs]
    values = [pair['ir_value'] for pair in beat_pairs]
    neg_values = [-value for value in values]

    # TODO: need to fix parameters 
    troughs, _ = find_peaks(neg_values, prominence = 200)
    return [{'from_start_device_time': timestamps[i], 'ir_value': values[i], 'peak': 0} for i in troughs]

def get_peaks(beat_pairs):
    timestamps = [pair['from_start_device_time'] for pair in beat_pairs]
    values = [pair['ir_value'] for pair in beat_pairs]

    # TODO: need to fix parameters 
    peaks, _ = find_peaks(values, prominence = 200)
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

app = initialize_app()

@db_fn.on_value_updated(reference="/heartbeat_data/is_uploading", region="asia-southeast1")
def processRecording(event: db_fn.Event[db_fn.Change]) -> None:
    """Listens for changes to the value of is_uploading and
    preprocesses the recordings then removes the recordings from the database
    """

    ref = db.reference('heartbeat_data')
    rec_ref = ref.child('recordings')
    is_uploading = ref.child('is_uploading').get()
    recordings = rec_ref.get()
    print(recordings)
    print(type(recordings)) # Debug
    print("length: ", len(recordings)) # Debug

    if is_uploading == 1 and recordings != None and len(recordings) > 0:
        recording_list = []
        for recording in recordings.values():
            recording_list.extend(recording)
        # preprocess
        ref.update({
            'preprocessed_heartbeat': {
                'spikes': calculate_delays(recording_list)
                }
            })
        # remove recordings
        ref.update({'recordings': None})
        ref.update({'is_vibrating': 1})
        ref.update({'is_uploading': 0})
        print('processed')
    # else:
        print("can't process")
    return True