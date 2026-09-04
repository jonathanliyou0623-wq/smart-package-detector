# Hardware notes

This page records what each device does, why it was selected, and what has actually been verified. It will be updated when part markings and physical measurements are available.

## ESP32 development board

**Known identification:** Arduino IDE board profile `ESP32 Dev Module`; exact board manufacturer/revision still to be recorded.

The ESP32 is the microcontroller at the center of the prototype. It runs the Arduino firmware, reads the sensor over I2C, applies the package-detection logic, and can later use its built-in Wi-Fi capability to send a notification.

Why it fits this project:

- enough processing power for filtering and a small state machine;
- built-in Wi-Fi for a future networked stage;
- inexpensive and supported by Arduino IDE; and
- easy serial logging during development.

**Verified so far:** the computer recognizes the board, firmware uploads successfully, and the Serial Monitor receives program output.

## VL53L0X distance sensor

**Model:** VL53L0X from STMicroelectronics. The exact breakout-board manufacturer will be recorded after physical inspection.

The VL53L0X is a **time-of-flight (ToF) distance sensor**. It emits invisible infrared light and estimates distance from the returning light. In this project, a package should make the measured distance shorter than the learned empty-door baseline.

Why it was selected:

- small module suitable for a doorway prototype;
- narrower sensing direction than a typical ultrasonic sensor;
- digital I2C communication with the ESP32; and
- does not require capturing identifiable camera images.

The reading can still vary because of ambient infrared light, surface color, angle, multi-path reflections, and normal measurement uncertainty. The firmware therefore filters samples and requires a change to persist before reporting an event.

**Verified so far:** the software dependency and ESP32 firmware compile in CI. Physical wiring and real-distance measurements are still pending.

## Mini solderless breadboard

**Type:** mini solderless breadboard; manufacturer/model number not yet identified.

The breadboard provides temporary electrical connections without soldering. It is useful while pin assignments and sensor placement are still changing. A more permanent version could later use a soldered prototyping board or custom enclosure.

## Dupont jumper wires

**Types available:** male-to-male, male-to-female, and female-to-female.

The connector type is chosen based on whether each board exposes header pins or breadboard sockets. For the first VL53L0X connection, the expected signals are 3.3 V, ground, SDA, and SCL.

## USB data cable

The USB cable supplies power during bench testing and carries firmware uploads and serial logs between the computer and ESP32. A charge-only cable would power the board but could not upload code, so successful uploading also verifies that the current cable supports data.

## Hardware record to complete

At the next physical session, record:

- the text printed on the ESP32 module and development board;
- the VL53L0X breakout-board manufacturer and pin labels;
- the measured empty-door distance and mounting angle; and
- representative readings for no package, a small box, and a large box.
