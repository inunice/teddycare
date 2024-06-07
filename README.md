# teddycare

TeddyCare is a smart cuddly toy that bridges the gap between parents and children, fostering heartfelt connections from anywhere to your home.

## Components

TeddyCare consists of two main components and a cloud function.

- **TeddyCare**. The `teddycare-bear-integrated` is for the stuffed toy itself. It facilitates heartbeat simulation, sound sensing capabilities, and song playback.

- **TeddyCare Bracelet**. The `teddycare-bracelet` is for the parent's bracelet. It features heartbeat recording and noise-activated notifications.

- **Heartbeat Data Preprocessor**. The `heartbeat-data-preprocessor` is to be deployed as a cloud function that communicates with the server. It converts raw data from the bracelet for the bear.
