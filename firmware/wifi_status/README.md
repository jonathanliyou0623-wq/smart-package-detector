# ESP32 simulated package detection

This separate sketch validates local network communication without a sensor.
It temporarily replaces the program on the board; the detector sketch remains
in its own directory.

1. Copy `secrets.example.h` to `secrets.h` and fill in your Wi-Fi credentials.
   `secrets.h` is ignored by Git; never force-add it.
2. Open `wifi_status.ino` in Arduino IDE and choose the board and serial port.
3. Upload and open Serial Monitor at 115200 baud.
4. Open the printed `http://.../` address from a computer on the same reachable
   local network. Guest/client isolation may prevent communication.

The page requests `/api/status` every half second and shows simulated distance,
filtered distance, detector state, baseline, arrival/removal counters, the latest
six events, uptime, and Wi-Fi signal. It explicitly reports simulation mode and
that no sensor is connected. It uses local HTTP
without authentication and is intended for this local experiment; do not expose
it through router port forwarding. No external page assets or cloud services
are required, and credentials are not returned by the server.

Validation of this page does not establish Internet access, notification delivery,
sensor operation, or recovery from network outages.

The board feeds the detector one sample approximately every 100 ms: 800 mm for
80 samples, 450 mm for 80 samples, then 800 mm for 80 samples, repeating. The
first 30 samples calibrate the baseline. Filtering and confirmation delay state
transitions. These are synthetic inputs, not measured distance data. Event times
are sample time (sample count times 100 ms); scheduling delays can make real time
slightly longer. `Restart simulation` sends a POST to `/api/simulation/reset`,
resetting calibration, counters, and history without restarting Wi-Fi.

The detector headers are exact copies from `../smart_package_detector/` because
Arduino builds each sketch in isolation. That directory remains the source of
truth: synchronize both headers after changes. CMake checks their hashes, and
the simulation test checks three cycles, confirmation delays, counters, and reset.
