# Real VL53L0X tabletop test

This record separates what was observed on the physical prototype from what remains to be tested at a doorway.

## Setup

- ESP32 Dev Module on USB power and local Wi-Fi
- VL53L0X breakout marked `VL53LXX-V2`
- VIN to 3V3, GND to GND, SDA to GPIO21, and SCL to GPIO22
- sensor aimed at a sheet of white paper on a tabletop
- tissue roll used as the repeatable test object
- tabletop profile: 60 mm detection delta, 30 mm clear delta, 30 valid calibration samples, and a fixed post-calibration baseline

The bare desktop repeatedly returned VL53L0X range status 2 in this arrangement. Keeping the sensor placement and adding white paper produced valid readings. This supports a surface-return explanation for this setup, but it was not a controlled optical characterization of the desk material.

## Observed results

The serial-only bring-up first demonstrated an empty background near 312 mm, the tissue roll near 205 mm with persistent `package_present` state, and a return near 314 mm with persistent `clear` state.

The integrated Wi-Fi firmware then completed another full cycle:

| Stage | Raw distance | Detector result | Event counts |
| --- | ---: | --- | --- |
| Empty scene after startup | 317 mm | No package | 0 arrivals / 0 removals |
| Tissue roll placed | 208–212 mm | Package detected | 1 arrival / 0 removals |
| Tissue roll removed | 318–319 mm | No package | 1 arrival / 1 removal |

The dashboard's captured post-test state showed a 315 mm live distance and baseline, 7,543 valid samples, zero invalid samples, and event history entries for baseline ready, package detected, and package removed.

![Live dashboard after the real sensor test](images/real-sensor-clear.png)

## Validity handling

Only VL53L0X measurements with API success and `RangeStatus == 0` enter calibration, filtering, or detection. Invalid returns are counted for diagnostics and cannot move the baseline or detector state.

## Limits and next work

This validates the electronics, measurement path, state machine, and local webpage in one controlled tabletop geometry. It does not establish ruler accuracy, maximum range, performance on different package materials, ambient-light tolerance, long-term stability, or reliability at a real doorway. The 60/30 mm thresholds are an experimental tabletop profile and should be retuned after collecting doorway data.
