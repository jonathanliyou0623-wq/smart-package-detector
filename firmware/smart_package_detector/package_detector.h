#pragma once

#include <cmath>
#include <cstdint>

#include "detector_config.h"

enum class DetectorState : uint8_t {
  Calibrating,
  Clear,
  PackagePresent,
};

enum class DetectorEvent : uint8_t {
  None,
  CalibrationComplete,
  PackageDetected,
  PackageRemoved,
};

class PackageDetector {
 public:
  explicit PackageDetector(DetectorConfig config = {}) : config_(config) {}

  DetectorEvent update(uint16_t distanceMm) {
    if (distanceMm == 0) {
      return DetectorEvent::None;
    }

    if (state_ == DetectorState::Calibrating) {
      calibrationTotal_ += distanceMm;
      ++calibrationCount_;
      if (calibrationCount_ >= config_.calibrationSamples) {
        baselineMm_ = static_cast<float>(calibrationTotal_) / calibrationCount_;
        filteredMm_ = baselineMm_;
        hasFilteredReading_ = true;
        state_ = DetectorState::Clear;
        return DetectorEvent::CalibrationComplete;
      }
      return DetectorEvent::None;
    }

    if (!hasFilteredReading_) {
      filteredMm_ = static_cast<float>(distanceMm);
      hasFilteredReading_ = true;
    } else {
      filteredMm_ += config_.measurementAlpha *
                     (static_cast<float>(distanceMm) - filteredMm_);
    }

    const float obstructionMm = baselineMm_ - filteredMm_;

    if (state_ == DetectorState::Clear) {
      if (obstructionMm >= config_.detectionDeltaMm) {
        ++transitionCount_;
        if (transitionCount_ >= config_.detectionSamples) {
          transitionCount_ = 0;
          state_ = DetectorState::PackagePresent;
          return DetectorEvent::PackageDetected;
        }
      } else {
        transitionCount_ = 0;
        baselineMm_ += config_.baselineAlpha * (filteredMm_ - baselineMm_);
      }
    } else {
      if (obstructionMm <= config_.clearDeltaMm) {
        ++transitionCount_;
        if (transitionCount_ >= config_.clearSamples) {
          transitionCount_ = 0;
          state_ = DetectorState::Clear;
          return DetectorEvent::PackageRemoved;
        }
      } else {
        transitionCount_ = 0;
      }
    }

    return DetectorEvent::None;
  }

  DetectorState state() const { return state_; }
  uint16_t baselineMm() const {
    return static_cast<uint16_t>(std::lround(baselineMm_));
  }
  uint16_t filteredMm() const {
    return static_cast<uint16_t>(std::lround(filteredMm_));
  }

 private:
  DetectorConfig config_;
  DetectorState state_ = DetectorState::Calibrating;
  uint32_t calibrationTotal_ = 0;
  uint16_t calibrationCount_ = 0;
  uint8_t transitionCount_ = 0;
  float baselineMm_ = 0.0f;
  float filteredMm_ = 0.0f;
  bool hasFilteredReading_ = false;
};
