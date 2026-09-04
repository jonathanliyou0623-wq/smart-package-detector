#include <Adafruit_VL53L0X.h>

#include "package_detector.h"

namespace {
constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kSampleIntervalMs = 100;

Adafruit_VL53L0X sensor;
PackageDetector detector;
uint32_t lastSampleAt = 0;

void printEvent(DetectorEvent event) {
  switch (event) {
    case DetectorEvent::CalibrationComplete:
      Serial.printf("Baseline ready: %u mm\n", detector.baselineMm());
      break;
    case DetectorEvent::PackageDetected:
      Serial.println("EVENT: package detected");
      break;
    case DetectorEvent::PackageRemoved:
      Serial.println("EVENT: package removed");
      break;
    case DetectorEvent::None:
      break;
  }
}
}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(500);
  Serial.println("\nSmart Package Detector");

  if (!sensor.begin()) {
    Serial.println("ERROR: VL53L0X not found. Check power and I2C wiring.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("VL53L0X online. Calibrating...");
}

void loop() {
  const uint32_t now = millis();
  if (now - lastSampleAt < kSampleIntervalMs) {
    return;
  }
  lastSampleAt = now;

  VL53L0X_RangingMeasurementData_t measurement;
  sensor.rangingTest(&measurement, false);

  if (measurement.RangeStatus == 4) {
    Serial.println("WARN: out-of-range sample ignored");
    return;
  }

  const uint16_t distanceMm = measurement.RangeMilliMeter;
  const DetectorEvent event = detector.update(distanceMm);

  Serial.printf("distance=%u mm filtered=%u mm baseline=%u mm\n",
                distanceMm,
                detector.filteredMm(),
                detector.baselineMm());
  printEvent(event);
}
