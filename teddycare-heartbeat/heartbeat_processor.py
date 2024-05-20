import threading
from google.cloud import firestore

def listen_document():
    db = firestore.Client()
    # Create an Event for notifying main thread.
    callback_done = threading.Event()

    # Create a callback on_snapshot function to capture changes
    def on_snapshot(doc_snapshot, changes, read_time):
        for doc in doc_snapshot:
            print(f"Received document snapshot: {doc.id}")
        #call preprocessing
        callback_done.set()

    doc_ref = db.collection("cities").document("SF")

    # Watch the document
    doc_watch = doc_ref.on_snapshot(on_snapshot)

    # Creating document
    data = {
        "average": average,
        "values": values,
    }
    doc_ref.set(data)
    # Wait for the callback.
    callback_done.wait(timeout=60)
    # [START firestore_listen_detach]
    # Terminate watch on a document
    doc_watch.unsubscribe()
    # [END firestore_listen_detach]
