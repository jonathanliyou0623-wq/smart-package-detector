#include <Adafruit_VL53L0X.h>

#include "package_detector.h"

namespace {
constexpr uint32_t kSerialBaud = 115200;
// Sample every 100 ms (10 Hz) / 每 100 毫秒采样一次，即 10 Hz。
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
  // setup() runs once after boot / setup() 在 ESP32 启动后只执行一次。
  Serial.begin(kSerialBaud);
  delay(500);
  Serial.println("\nSmart Package Detector");

  if (!sensor.begin()) {
    // Fail visibly instead of continuing with fake data.
    // 找不到传感器时明确停止，避免使用无效数据继续判断。
    Serial.println("ERROR: VL53L0X not found. Check power and I2C wiring.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("VL53L0X online. Calibrating...");
}

void loop() {
  // Non-blocking timing / 非阻塞计时：不使用长 delay，方便以后加入联网任务。
  const uint32_t now = millis();
  if (now - lastSampleAt < kSampleIntervalMs) {
    return;
  }
  lastSampleAt = now;

  VL53L0X_RangingMeasurementData_t measurement;
  // Read one ToF measurement / 从 VL53L0X 读取一次飞行时间测距结果。
  sensor.rangingTest(&measurement, false);

  if (measurement.RangeStatus == 4) {
    // Status 4 means out of range / 状态 4 表示超出有效测量范围。
    Serial.println("WARN: out-of-range sample ignored");
    return;
  }

  const uint16_t distanceMm = measurement.RangeMilliMeter;
  // Hardware I/O ends here; decision logic is independently testable.
  // 硬件读取到此结束；判断逻辑放在独立类中，便于在电脑上测试。
  const DetectorEvent event = detector.update(distanceMm);

  Serial.printf("distance=%u mm filtered=%u mm baseline=%u mm\n",
                distanceMm,
                detector.filteredMm(),
                detector.baselineMm());
  printEvent(event);
}
