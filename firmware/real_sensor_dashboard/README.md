# Real sensor Wi-Fi dashboard

1. Wire the VL53L0X as documented in [`../../docs/wiring.md`](../../docs/wiring.md).
2. Copy `secrets.example.h` to `secrets.h` and enter the local Wi-Fi name and password. `secrets.h` is ignored by Git.
3. Install the Adafruit VL53L0X Arduino library.
4. Open `real_sensor_dashboard.ino`, select **ESP32 Dev Module**, and upload.
5. Keep the sensor view empty for the first three seconds, then open the local address printed at 115200 baud.

The current thresholds are tuned for the controlled tabletop experiment. An
obstruction must persist for about five seconds before an arrival is reported.
Use **Recalibrate empty background** whenever the sensor or background moves, and
keep the scene empty until calibration finishes.

The webpage also provides **Download measurements CSV** and three manual action
markers. The device retains the latest 1,200 rows in RAM (roughly two minutes
at 10 Hz); older rows are overwritten and power loss clears the history. See the
[CSV field guide](../../docs/data-logging.md) before using the data for timing or
accuracy claims.
