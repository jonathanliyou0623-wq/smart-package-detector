# Real VL53L0X tabletop test

This record separates what was observed on the physical prototype from what remains to be tested at a doorway.

## Setup

- ESP32 Dev Module on USB power and local Wi-Fi
- VL53L0X breakout marked `VL53LXX-V2`
- VIN to 3V3, GND to GND, SDA to GPIO21, and SCL to GPIO22
- sensor aimed at a sheet of white paper on a tabletop
- tissue roll used as the repeatable test object
- tabletop profile used for the recorded milestone: 60 mm detection delta, 30 mm clear delta, 30 valid calibration samples, and a fixed post-calibration baseline

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

## Five-second arrival confirmation (September 22)

Arrival confirmation was increased from 8 to 50 qualifying samples. At the
approximately 10 Hz sample rate, this calls for about five seconds of sustained
obstruction; the 12-sample removal confirmation remains about 1.2 seconds.

- A user-held hand obstruction lasting approximately 2–3 seconds did not trigger
  an arrival: the state remained `No package` and both event counters stayed at zero.
- The first tissue-roll trial did not trigger. After 709 invalid readings, the
  sensor resumed valid ranging and calibrated at 201 mm, matching the roll's
  approximately 203 mm reading. The exact moment the roll entered view was not
  timestamped. This trial does not test the arrival delay.
- The roll was removed and calibration was restarted through the API with the
  white-paper background empty. The new baseline was 312 mm, with zero invalid
  readings after the restart.
- The roll was placed again. Three API reads showed 203–207 mm, `Package detected`,
  one arrival and zero removals. The event history recorded detection at device
  uptime 355 s. After removal, three reads showed 312–316 mm, `No package`, one
  arrival and one removal; the removal event was recorded at uptime 474 s.

These checks show that this short hand obstruction was filtered and a sustained
roll placement produced one arrival/removal cycle. Placement and removal times
were not timestamped independently, so the experiment does not directly measure
the actual detection or removal latency. The sensor still cannot tell a package
from another object that stays in view for long enough.

## Timestamped labeled trial (September 23)

The dashboard's RAM ring buffer and manual marker controls were used for a first
timestamped trial set. The setup remained the same: a white-paper background and
a tissue roll as the test object. The detector baseline was 305 mm.

| Trial | Observed samples | Result |
| --- | --- | --- |
| Brief hand pass | Minimum raw distance 87 mm; 26 consecutive qualifying samples, approximately 2.6 s | Rejected; state stayed `clear` and no arrival event occurred |
| Tissue roll placed | Filtered distance first crossed the 245 mm arrival threshold at 244 mm | Detected after 50 qualifying samples and 4.907 s; event distance 225 mm |
| Tissue roll removed | Filtered distance first crossed the 275 mm clear threshold at 276 mm | Cleared after 12 qualifying samples and 1.100 s; event distance 327 mm |

The removal marker preceded the removal event by 2.499 seconds. That interval
includes the user's physical action; the 1.100-second value starts when the
filtered measurement first satisfied the clear condition. The placement marker
was overwritten before download, so the 4.907-second value likewise measures the
detector from its first qualifying sample, not from the user's click.

All 1,200 retained rows in the placement and removal downloads were valid sensor
returns, and the completed cycle produced one arrival and one removal. This is
evidence for the configured confirmation timing in one controlled cycle. It is
not enough to report precision, recall, overall accuracy, or false positives per
hour; those require repeated labeled objects, non-package obstructions, and
longer unattended runs.

### Three-cycle repeatability check

Three additional placement/removal cycles were run with the same 305 mm baseline
and tissue roll. Every placement produced one arrival transition and every
removal produced one clear transition.

| Measurement | Trials | Mean | Range | Sample standard deviation |
| --- | ---: | ---: | ---: | ---: |
| Arrival confirmation after first qualifying filtered sample | 3 | 4.941 s | 4.910–5.004 s | 0.054 s |
| Removal confirmation after first qualifying filtered sample | 3 | 1.101 s | 1.100–1.102 s | 0.001 s |

The three placement trials and three removal trials therefore completed 3/3
expected state transitions in this fixed geometry. Two removal marker rows were
not present in the downloaded CSV, but their sensor samples and removal events
were retained and support the algorithm-delay measurements. Human marker-to-event
time is excluded from the summary because it includes the time needed to move the
object into or out of the sensor beam.

This small repeated set demonstrates deterministic confirmation timing for one
object and setup. It still cannot be interpreted as a general detection accuracy
or false-positive rate.
