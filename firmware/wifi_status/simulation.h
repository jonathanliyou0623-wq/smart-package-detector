#pragma once

#include "package_detector.h"

// One sample per 100 ms. Input changes; the detector decides the state.
class Simulation {
 public:
  static constexpr uint16_t kCycleSamples = 240;
  DetectorEvent tick() {
    phase_ = (samples_ % kCycleSamples) / 80;
    distanceMm_ = phase_ == 1 ? 450 : 800;
    ++samples_;
    const DetectorEvent event = detector_.update(distanceMm_);
    if (event == DetectorEvent::PackageDetected) ++arrivals_;
    if (event == DetectorEvent::PackageRemoved) ++removals_;
    return event;
  }
  const PackageDetector& detector() const { return detector_; }
  uint16_t distanceMm() const { return distanceMm_; }
  uint8_t phase() const { return phase_; }
  uint32_t samples() const { return samples_; }
  uint32_t arrivals() const { return arrivals_; }
  uint32_t removals() const { return removals_; }

 private:
  PackageDetector detector_;
  uint32_t samples_ = 0;
  uint32_t arrivals_ = 0;
  uint32_t removals_ = 0;
  uint16_t distanceMm_ = 0;
  uint8_t phase_ = 0;
};
