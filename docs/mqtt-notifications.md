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
events to an eight-entry fixed RAM queue. The Arduino network loop connects to
the broker, publishes the oldest event, and removes that event only when the
MQTT client accepts the publish operation. A failed connection or local publish
keeps the event for a later attempt. If the queue fills, the oldest pending
events are preserved, newer events are rejected, and the dropped counter rises.

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

## End-to-end validation still required

The disabled path has been uploaded and verified on the physical ESP32. The
enabled path and fixed queue compile successfully, and the queue has host-side
tests for FIFO order, overflow, retries, and stale acknowledgements. A real
broker is still required for these hardware checks:

1. Confirm TLS connection and one arrival/removal payload pair.
2. Disconnect Wi-Fi, trigger an event, and verify `mqtt_pending` increases.
3. Restore Wi-Fi and verify the queued event publishes once connectivity returns.
4. Confirm no credentials or local configuration appear in `git status`.

Do not claim live notification delivery until those checks pass with the chosen
broker.
