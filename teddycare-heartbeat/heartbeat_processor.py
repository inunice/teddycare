from time import sleep
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from beat_sample import get_bpm, get_changepoints

cred = credentials.Certificate('path/to/serviceKey.json')

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
    if not is_recording and recordings != None and len(recordings) == 30:
        recording_list = []
        for recording in recordings.values():
            recording_list.extend(recording)
        recording_list.sort(key = lambda pair: pair[0])
        # preprocess
        ref.update({
            'preprocessed_heartbeat': {
                'frequency': get_bpm(recording_list),
                'peaks': get_changepoints(recording_list),
                }
            })
        ref.update({'recordings': None})
    sleep(60)



