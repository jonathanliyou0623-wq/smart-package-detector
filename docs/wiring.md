# VL53L0X wiring and bring-up

> Planned first-prototype wiring / 第一版计划接线。The pin map must be checked against the labels printed on the actual breakout board before power is connected.

## Pin map

Connect the board with USB power disconnected.

| VL53L0X | ESP32 Dev Module | Purpose |
| --- | --- | --- |
| VIN | 3V3 | Sensor power |
| GND | GND | Common ground |
| SDA | GPIO 21 | I2C data |
| SCL | GPIO 22 | I2C clock |

`XSHUT` and `GPIO1/INT` are not required for the first prototype. Some breakout boards accept 5 V on VIN, but 3.3 V is the conservative choice unless the specific board documentation says otherwise.

## Physical placement

### Current tabletop test profile

The sensor sketch currently uses `desktop_test_config.h`, tuned for the measured
paper-covered table (~311 mm) and tissue roll (~200 mm). It detects a sustained
60 mm decrease and clears when the remaining decrease is at most 30 mm. The
tabletop profile now requires 50 consecutive arrival samples (about 5 seconds at
10 Hz) and 12 removal samples (about 1.2 seconds).
The baseline is fixed after startup calibration for this controlled experiment;
restart with the scene empty whenever the sensor or background moves. These are
experimental tabletop settings, not validated doorway settings. The Wi-Fi
simulation retains its original configuration.

- Aim the sensor at a fixed background surface, not open space.
- Keep the expected package location inside the sensor's field of view.
- Mount the sensor rigidly; small angle changes can shift the baseline.
- Start with the background between roughly 300 mm and 1200 mm away.

## Bring-up checklist

1. Check the pin labels on the actual sensor board.
2. Connect power, ground, SDA, and SCL with USB disconnected.
3. Upload the firmware and open Serial Monitor at 115200 baud.
4. Keep the sensing area empty during the startup calibration.
5. Confirm that the baseline is stable before placing an object.
6. Record false triggers and adjust values in `detector_config.h` only after collecting data.

## Troubleshooting

If the firmware prints `VL53L0X not found`, disconnect USB and check power, ground, and the two I2C lines. If readings are constantly out of range, move the target closer and avoid highly reflective, transparent, or very dark surfaces during initial testing.

The sensor sketch accepts a measurement only when the API call succeeds and
`RangeStatus` is `0`. Valid serial readings end with `status=0`. A
`WARN: invalid range status=... raw_mm=...; sample ignored` line is diagnostic:
`raw_mm` is not a valid distance and does not update filtering, calibration, or
package detection. Status `2` means the return signal is too weak (signal fail).
Try a broad matte target in front of the optical window and check for obstruction.
An `api_error` warning instead means the measurement API call failed.

Each valid line also reports the detector state: `calibrating`, `clear`, or
`package_present`. This persistent state makes the result observable even if a
one-time `EVENT:` line occurred before Serial Monitor was opened.
