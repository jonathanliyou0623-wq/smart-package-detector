# MQTT/TLS notifications

The real-sensor firmware can publish package arrival and removal events to an
MQTT 3.1.1 broker over TLS. Notifications are disabled by default, so a fresh
checkout never attempts an external connection or contains broker credentials.

## Configure locally

1. Copy
   `firmware/real_sensor_dashboard/mqtt_config.local.example.h` to
   `mqtt_config.local.h` in the same directory.
2. Enter the broker hostname, TLS port, username, password, topic prefix, and
   the broker's published root CA certificate.
3. Keep `mqtt_config.local.h` local. The repository ignores `*.local.h` files.
4. Compile and upload the real-sensor dashboard.

Do not replace certificate validation with `setInsecure()`. The hostname and
root CA must match the selected broker. The current implementation expects a
TLS MQTT endpoint, normally on port 8883.

When MQTT is enabled, the ESP32 synchronizes UTC time with NTP before opening a
TLS connection so certificate validity dates can be checked. Until time is
available, the API reports `clock_not_synced` and keeps queued events pending.

Events are published to `<MQTT_TOPIC_PREFIX>/events` as compact JSON:

```json
{
  "sequence": 1,
  "event": "package_detected",
  "uptime_ms": 4209013,
  "raw_mm": 249,
  "filtered_mm": 248,
  "baseline_mm": 335,
  "attempt": 1
}
```

`sequence` increases within one device boot. `uptime_ms` is device uptime, not
wall-clock time. Removal uses `"event":"package_removed"`.

## Runtime behavior and limits

The sensor task never performs network I/O. It appends arrival and removal
events to an eight-entry fixed RAM queue. A separate lower-priority MQTT task on
ESP32 core 0 connects to the broker, services the client, publishes the oldest
event, and removes that event only when the MQTT client accepts the publish
operation. If the MQTT task cannot be created, the Arduino loop provides a
fallback service path. A failed connection or local publish keeps the event for
a later attempt. If the queue fills, the oldest pending events are preserved,
newer events are rejected, and the dropped counter rises.

The queue is stored only in RAM and is lost on reboot or power loss. PubSubClient
publishes at QoS 0, so a successful local call is not a broker acknowledgement;
an event can still be lost if the connection fails at the wrong moment. The
queue improves short outage handling but does not claim exactly-once delivery.

The dashboard and `/api/status` expose:

| Field | Meaning |
| --- | --- |
| `mqtt_enabled` | Whether a local MQTT configuration was compiled in |
| `mqtt_connected` | Current broker connection state |
| `mqtt_pending` | Events waiting in the RAM queue |
| `mqtt_dropped` | Events rejected because the queue was full |
| `mqtt_published` | Events accepted by the MQTT client's publish call |
| `mqtt_failures` | Failed connect, connection, or publish operations |
| `mqtt_last_error` | Short local diagnostic without credentials |

## End-to-end validation

On October 8, 2026, the enabled firmware was compiled and uploaded to the
physical ESP32 with a local, ignored EMQX Cloud Serverless configuration. The
device synchronized its clock, completed certificate-validated TLS, and reported
`mqtt_connected=true`. A separate MQTTX Web client subscribed over WSS to
`smart-package-detector/esp32-01/events` and received both sides of a controlled
gum-container cycle:

| Sequence | Event | Raw | Filtered | Baseline | Attempt |
| ---: | --- | ---: | ---: | ---: | ---: |
| 3 | `package_detected` | 236 mm | 237 mm | 323 mm | 1 |
| 4 | `package_removed` | 321 mm | 322 mm | 323 mm | 1 |

After the pair, the device reported four publishes since boot, zero queued
events, zero drops, and zero failures. Subscriber observation confirms broker
delivery for this controlled test rather than relying only on PubSubClient's
local QoS 0 return value. The local configuration remained excluded by
`*.local.h`; credentials are not present in the repository.

The same day, broker-outage recovery was tested on hardware. EMQX was stopped
while the ESP32 remained powered. A gum-container arrival left
`mqtt_published=4` and increased `mqtt_pending` from zero to one, with zero
drops. After EMQX restarted, the ESP32 reconnected automatically,
`mqtt_pending` returned to zero, and `mqtt_published` increased to five. EMQX's
new ESP32 session recorded one received PUBLISH, one received QoS 0 message, and
zero dropped incoming messages. This broker-side count confirms receipt of the
queued event even though the browser subscriber was also disconnected during
the outage and therefore could not display that non-retained QoS 0 message.

After MQTTX reconnected, removing the container produced sequence 6
`package_removed`; MQTTX displayed the message and the device reported six
publishes, zero pending events, and zero drops. The stop/queue/reconnect path is
therefore validated for one controlled outage cycle.

The initial implementation performed MQTT connection attempts in the Arduino
web loop. One 10-second `/api/status` request timed out while the broker was
starting, even though the independent sensor task continued sampling. MQTT/TLS
service was therefore moved to its own FreeRTOS task. A 30-request online
baseline then completed without failures (103.8 ms average, 137.9 ms
95th-percentile, 241.5 ms maximum). With EMQX stopped, 60 of 60 status requests
again succeeded despite repeated connection failures: average latency was
111.7 ms, 95th-percentile latency was 186.7 ms, and the maximum was 269.3 ms.
After EMQX restarted, the device reconnected automatically without a reboot.
These results validate dashboard responsiveness for this controlled outage and
local network, rather than every possible Wi-Fi or TLS failure mode.
