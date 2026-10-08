# Real sensor Wi-Fi dashboard

1. Wire the VL53L0X as documented in [`../../docs/wiring.md`](../../docs/wiring.md).
2. Copy `secrets.example.h` to `secrets.h` and enter the local Wi-Fi name and password. `secrets.h` is ignored by Git.
3. Install the Adafruit VL53L0X Arduino library.
4. Install the PubSubClient library. MQTT remains disabled unless a local MQTT
   configuration is provided.
5. Optionally copy `mqtt_config.local.example.h` to `mqtt_config.local.h` and
   enter a TLS broker configuration. See the
   [MQTT notification guide](../../docs/mqtt-notifications.md).
6. Open `real_sensor_dashboard.ino`, select **ESP32 Dev Module**, and upload.
7. Keep the sensor view empty for the first three seconds, then open the local address printed at 115200 baud.

The current thresholds are tuned for the controlled tabletop experiment. An
obstruction must persist for about five seconds before an arrival is reported.
Use **Recalibrate empty background** whenever the sensor or background moves, and
keep the scene empty until calibration finishes.

The webpage also provides **Download measurements CSV** and three manual action
markers. The device retains the latest 1,200 rows in RAM (roughly two minutes
at 10 Hz); older rows are overwritten and power loss clears the history. See the
[CSV field guide](../../docs/data-logging.md) before using the data for timing or
accuracy claims.

Sensor acquisition runs in a dedicated FreeRTOS task on ESP32 core 1. HTTP
handlers take short mutex-protected snapshots of detector state or the ring
buffer and release the mutex before sending network data, so a slow CSV client
does not block the normal 100 ms measurement schedule.

When MQTT is enabled, arrival and removal events enter an eight-record RAM queue.
The network loop publishes the oldest event over TLS and retries local failures
without blocking sensor acquisition. Queue health and connection state appear on
the dashboard and status API. Broker delivery has not been validated until the
hardware tests in the MQTT guide are completed.
