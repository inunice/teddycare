import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from scipy.signal import find_peaks
from math import ceil

def normalize(y, old_min, old_max, new_min = 150, new_max = 255):
    a = (new_max - new_min) / (old_max - old_min)
    b = new_max - a * old_max
    return min(max(ceil(a * y + b), new_min), 255)

def get_peaks(beat_pairs, normal = False):
    timestamps = [pair['from_start_device_time'] for pair in beat_pairs]
    values = [pair['ir_value'] for pair in beat_pairs]

    # normalize values

    # TODO: need to fix parameters 
    peaks, _ = find_peaks(values, distance = 10)
    # peaks = list(peaks)
    if normal:
        max_val = max(values[i] for i in peaks)
        min_val = min(values[i] for i in peaks)
        return [{'from_start_device_time': timestamps[i], 'ir_value': normalize(values[i], min_val, max_val)} for i in peaks]
    else:
        return [{'from_start_device_time': timestamps[i], 'ir_value': values[i]} for i in peaks]

def get_bpm(beat_pairs):
    num_peaks = len(get_peaks(beat_pairs))
    return num_peaks * 2



cred = credentials.Certificate('test1-a4e94-firebase-adminsdk-mv0uj-28a74beeb8.json')

# Initialize the app with a service account, granting admin privileges
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://test1-a4e94-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })


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
                'frequency': get_bpm(recording_list),
                'peaks': get_peaks(recording_list, normal = True),
                }
            })
        # remove recordings
        ref.update({'recordings': None})
        print('processed')
