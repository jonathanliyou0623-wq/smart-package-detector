#include <cstdlib>
#include <iostream>

#include "desktop_test_config.h"
#include "package_detector.h"

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

int main() {
  const auto config = desktopTestConfig();
  PackageDetector detector(config);
  for (int i = 0; i < config.calibrationSamples; ++i) detector.update(311);
  expect(detector.state() == DetectorState::Clear, "empty scene calibrates");
  // Recorded empty-table and tissue-roll ranges, including a four-second
  // obstruction that must not be treated as a delivery.
  for (int i = 0; i < 100; ++i) detector.update(306 + i % 10);
  for (int i = 0; i < 40; ++i) detector.update(200);
  expect(detector.state() == DetectorState::Clear,
         "four-second obstruction is ignored");
  for (int i = 0; i < 50; ++i) detector.update(311);
  expect(detector.state() == DetectorState::Clear, "clear scene resets confirmation");
  int arrivals = 0;
  for (int i = 0; i < 100; ++i)
    arrivals += detector.update(196 + i % 9) == DetectorEvent::PackageDetected;
  expect(arrivals == 1, "measured roll range generates one arrival");
  for (int i = 0; i < 600; ++i) detector.update(196 + i % 9);
  expect(detector.state() == DetectorState::PackagePresent, "roll remains present");
  expect(detector.baselineMm() == 311, "roll is not absorbed into background");
  int removals = 0;
  for (int i = 0; i < 100; ++i)
    removals += detector.update(306 + i % 10) == DetectorEvent::PackageRemoved;
  expect(removals == 1, "restored background generates one removal");
  expect(detector.state() == DetectorState::Clear, "scene returns to clear");
  std::cout << "Desktop profile tests passed.\n";
}
