from time import sleep
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from beat_sample import get_bpm, get_peaks

cred = credentials.Certificate('test1-a4e94-firebase-adminsdk-mv0uj-28a74beeb8.json')

# Initialize the app with a service account, granting admin privileges
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://test1-a4e94-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

# As an admin, the app has access to read and write all data, regradless of Security Rules
ref = db.reference('heartbeat_data')
while True:
    rec_ref = ref.child('recordings')
    is_recording = ref.child('is_recording').get()
    recordings = rec_ref.get()
    
    #test
    recording_list = []
    for recording in recordings.values():
        recording_list.extend(recording[:-1])

    break
    if is_recording == '0' and recordings != None and len(recordings) > 0:
        recording_list = []
        for recording in recordings.values():
            recording_list.extend(recording)
        # preprocess
        ref.update({
            'preprocessed_heartbeat': {
                'frequency': get_bpm(recording_list),
                'peaks': get_peaks(recording_list),
                }
            })
        # ref.update({'recordings': None})
        print('processed')
    sleep(60)

import matplotlib.pyplot as plt
beats = recording_list
beats_x = [pair['from_start_device_time'] for pair in beats]
beats_y = [pair['ir_value'] for pair in beats]
print(beats_x)
print(beats_y)

peaks = get_peaks(beats)
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


