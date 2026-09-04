# VL53L0X wiring and bring-up

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
