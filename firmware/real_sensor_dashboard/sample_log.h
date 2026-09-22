#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "package_detector.h"

enum class SampleQuality : uint8_t {
  Valid,
  RangeInvalid,
  ApiError,
  SensorOffline,
  CalibrationReset,
  ManualMarker,
};

enum class ExperimentMarker : uint8_t {
  None,
  HandPass,
  ObjectPlaced,
  ObjectRemoved,
};

struct SampleRecord {
  uint64_t uptimeMs = 0;
  uint16_t rawMm = 0;
  uint16_t filteredMm = 0;
  uint16_t baselineMm = 0;
  int16_t apiError = 0;
  uint8_t rangeStatus = 255;
  DetectorState state = DetectorState::Calibrating;
  DetectorEvent event = DetectorEvent::None;
  SampleQuality quality = SampleQuality::SensorOffline;
  ExperimentMarker marker = ExperimentMarker::None;
};

// Fixed RAM use: roughly two minutes of 10 Hz samples. Older rows are replaced.
class SampleLog {
 public:
  static constexpr size_t kCapacity = 1200;

  void append(const SampleRecord& record) {
    records_[next_] = record;
    next_ = (next_ + 1) % kCapacity;
    if (count_ < kCapacity) ++count_;
  }

  size_t size() const { return count_; }

  const SampleRecord& oldest(size_t index) const {
    return records_[(next_ + kCapacity - count_ + index) % kCapacity];
  }

 private:
  SampleRecord records_[kCapacity]{};
  size_t next_ = 0;
  size_t count_ = 0;
};

inline const char* sampleQualityName(SampleQuality quality) {
  switch (quality) {
    case SampleQuality::Valid: return "valid";
    case SampleQuality::RangeInvalid: return "range_invalid";
    case SampleQuality::ApiError: return "api_error";
    case SampleQuality::SensorOffline: return "sensor_offline";
    case SampleQuality::CalibrationReset: return "calibration_reset";
    case SampleQuality::ManualMarker: return "manual_marker";
  }
  return "unknown";
}

inline const char* loggedStateName(DetectorState state) {
  switch (state) {
    case DetectorState::Calibrating: return "calibrating";
    case DetectorState::Clear: return "clear";
    case DetectorState::PackagePresent: return "package_present";
  }
  return "unknown";
}

inline const char* loggedEventName(DetectorEvent event) {
  switch (event) {
    case DetectorEvent::None: return "none";
    case DetectorEvent::CalibrationComplete: return "baseline_ready";
    case DetectorEvent::PackageDetected: return "package_detected";
    case DetectorEvent::PackageRemoved: return "package_removed";
  }
  return "unknown";
}

inline const char* experimentMarkerName(ExperimentMarker marker) {
  switch (marker) {
    case ExperimentMarker::None: return "none";
    case ExperimentMarker::HandPass: return "hand_pass";
    case ExperimentMarker::ObjectPlaced: return "object_placed";
    case ExperimentMarker::ObjectRemoved: return "object_removed";
  }
  return "unknown";
}

inline int formatSampleCsvRow(char* output, size_t outputSize,
                              const SampleRecord& record) {
  return std::snprintf(output, outputSize, "%llu,%s,%d,%u,%u,%u,%u,%s,%s,%s\n",
                       static_cast<unsigned long long>(record.uptimeMs),
                       sampleQualityName(record.quality),
                       static_cast<int>(record.apiError),
                       static_cast<unsigned int>(record.rangeStatus),
                       static_cast<unsigned int>(record.rawMm),
                       static_cast<unsigned int>(record.filteredMm),
                       static_cast<unsigned int>(record.baselineMm),
                       loggedStateName(record.state),
                       loggedEventName(record.event),
                       experimentMarkerName(record.marker));
}
