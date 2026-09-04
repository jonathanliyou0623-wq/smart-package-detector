#include <cstdlib>
#include <iostream>

#include "package_detector.h"

namespace {
void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

DetectorConfig testConfig() {
  DetectorConfig config;
  config.detectionDeltaMm = 100;
  config.clearDeltaMm = 40;
  config.detectionSamples = 3;
  config.clearSamples = 3;
  config.calibrationSamples = 5;
  config.measurementAlpha = 1.0f;
  config.baselineAlpha = 0.0f;
  return config;
}

void calibrate(PackageDetector& detector, uint16_t distanceMm = 800) {
  DetectorEvent event = DetectorEvent::None;
  for (int i = 0; i < 5; ++i) {
    event = detector.update(distanceMm);
  }
  expect(event == DetectorEvent::CalibrationComplete,
         "calibration should complete after five samples");
}

void testCalibration() {
  PackageDetector detector(testConfig());
  calibrate(detector);
  expect(detector.state() == DetectorState::Clear,
         "detector should be clear after calibration");
  expect(detector.baselineMm() == 800, "baseline should equal sample mean");
}

void testSustainedObstructionTriggersDetection() {
  PackageDetector detector(testConfig());
  calibrate(detector);
  expect(detector.update(650) == DetectorEvent::None, "first sample should debounce");
  expect(detector.update(650) == DetectorEvent::None, "second sample should debounce");
  expect(detector.update(650) == DetectorEvent::PackageDetected,
         "third sustained sample should detect package");
}

void testBriefObstructionIsIgnored() {
  PackageDetector detector(testConfig());
  calibrate(detector);
  detector.update(650);
  detector.update(650);
  detector.update(800);
  expect(detector.state() == DetectorState::Clear,
         "brief obstruction should not change state");
}

void testRemovalRequiresStableClearance() {
  PackageDetector detector(testConfig());
  calibrate(detector);
  for (int i = 0; i < 3; ++i) detector.update(650);
  expect(detector.state() == DetectorState::PackagePresent,
         "package should be present before removal test");
  detector.update(800);
  detector.update(800);
  expect(detector.update(800) == DetectorEvent::PackageRemoved,
         "stable clearance should report removal");
  expect(detector.state() == DetectorState::Clear,
         "detector should return to clear state");
}
}  // namespace

int main() {
  testCalibration();
  testSustainedObstructionTriggersDetection();
  testBriefObstructionIsIgnored();
  testRemovalRequiresStableClearance();
  std::cout << "All package detector tests passed.\n";
  return 0;
}
