# Hardware notes

This page records what each device does, why it was selected, and what has actually been verified. It will be updated when part markings and physical measurements are available.

## ESP32 development board

**Known identification:** Arduino IDE board profile `ESP32 Dev Module`; exact board manufacturer/revision still to be recorded.

The ESP32 is the microcontroller at the center of the prototype. It reads the sensor over I2C, runs the package-detection logic, and serves a local Wi-Fi dashboard. Delivery notifications are a later stage.

Why it fits this project:

- enough processing power for filtering and a small state machine;
- built-in Wi-Fi for a future networked stage;
- inexpensive and supported by Arduino IDE; and
- easy serial logging during development.

**Verified through September 21, 2026:** firmware upload, serial output, Wi-Fi, local HTTP communication, VL53L0X I2C measurements, and real tabletop arrival/removal detection. Both the earlier [simulation](simulation.md) and the [real sensor test](real-sensor-test.md) report calibration, arrival, and removal on the physical ESP32.

## VL53L0X distance sensor

**Model:** VL53L0X from STMicroelectronics on a breakout marked `VL53LXX-V2`; manufacturer is unknown.

The VL53L0X is a **time-of-flight (ToF) distance sensor**. It emits invisible infrared light and estimates distance from the returning light. In this project, a package should make the measured distance shorter than the learned empty-door baseline.

Why it was selected:

- small module suitable for a doorway prototype;
- narrower sensing direction than a typical ultrasonic sensor;
- digital I2C communication with the ESP32; and
- does not require capturing identifiable camera images.

The reading can still vary because of ambient infrared light, surface color, angle, multi-path reflections, and normal measurement uncertainty. The firmware therefore filters samples and requires a change to persist before reporting an event.

**Current status:** the header has been soldered and checked for solder bridges. The board is connected to ESP32 3V3, GND, GPIO21/SDA, and GPIO22/SCL. Real valid readings, baseline calibration, arrival detection, removal detection, and the local Wi-Fi dashboard have been verified in a controlled white-paper tabletop setup. Doorway placement and broader target testing remain open.

## Mini solderless breadboard

**Type:** mini solderless breadboard; manufacturer/model number not yet identified.

The breadboard provides temporary electrical connections without soldering. It is useful while pin assignments and sensor placement are still changing. A more permanent version could later use a soldered prototyping board or custom enclosure.

## Dupont jumper wires

**Types available:** male-to-male, male-to-female, and female-to-female.

The connector type is chosen based on whether each board exposes header pins or breadboard sockets. For the first VL53L0X connection, the expected signals are 3.3 V, ground, SDA, and SCL.

## USB data cable

The USB cable supplies power during bench testing and carries firmware uploads and serial logs between the computer and ESP32. A charge-only cable would power the board but could not upload code, so successful uploading also verifies that the current cable supports data.

## Hardware record to complete

At a future doorway session, record:

- the text printed on the ESP32 module and development board;
- the measured empty-door distance and mounting angle; and
- representative readings for no package, a small box, and a large box.
