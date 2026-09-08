#include <cstdlib>
#include <iostream>
#include "simulation.h"

void require(bool condition, const char* message) {
  if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

int main() {
  Simulation sim;
  unsigned int calibration = 0, arrivals = 0, removals = 0;
  for (unsigned int i = 0; i < 3 * Simulation::kCycleSamples; ++i) {
    const auto event = sim.tick();
    if (event == DetectorEvent::CalibrationComplete) ++calibration;
    if (event == DetectorEvent::PackageDetected) {
      ++arrivals;
      require(sim.phase() == 1, "Arrival must occur during object input");
    }
    if (event == DetectorEvent::PackageRemoved) {
      ++removals;
      require(sim.phase() == 2, "Removal must occur during cleared input");
    }
    if (i % 240 == 80) {
      require(sim.detector().state() == DetectorState::Clear,
              "First near sample must not immediately declare a package");
    }
    if (i % 240 == 160) {
      require(sim.detector().state() == DetectorState::PackagePresent,
              "First far sample must not immediately declare removal");
    }
  }
  require(calibration == 1 && arrivals == 3 && removals == 3,
          "Three cycles must produce one calibration and three event pairs");
  require(sim.arrivals() == arrivals && sim.removals() == removals,
          "Displayed counters must match detector events");
  sim = Simulation{};
  require(sim.detector().state() == DetectorState::Calibrating &&
          sim.samples() == 0 && sim.arrivals() == 0 && sim.removals() == 0,
          "Restart must clear detector and counters");
  std::cout << "Simulation: three cycles, confirmation delay, and reset passed.\n";
}
