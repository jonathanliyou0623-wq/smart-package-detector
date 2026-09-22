#include <cstdlib>
#include <cstring>
#include <iostream>

#include "sample_log.h"

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

int main() {
  SampleLog log;
  expect(log.size() == 0, "log starts empty");

  SampleRecord record;
  for (size_t i = 0; i < SampleLog::kCapacity + 3; ++i) {
    record.uptimeMs = i * 100;
    record.rawMm = 200;
    record.filteredMm = 205;
    record.baselineMm = 312;
    record.rangeStatus = 0;
    record.state = DetectorState::PackagePresent;
    record.quality = SampleQuality::Valid;
    record.event = i == SampleLog::kCapacity + 2
                       ? DetectorEvent::PackageDetected
                       : DetectorEvent::None;
    log.append(record);
  }
  expect(log.size() == SampleLog::kCapacity, "log stays bounded");
  expect(log.oldest(0).uptimeMs == 300, "oldest samples are overwritten");
  expect(log.oldest(log.size() - 1).uptimeMs ==
             (SampleLog::kCapacity + 2) * 100,
         "newest sample is retained");

  char row[160];
  const int length = formatSampleCsvRow(row, sizeof(row),
                                        log.oldest(log.size() - 1));
  expect(length > 0 && static_cast<size_t>(length) < sizeof(row),
         "CSV row fits fixed buffer");
  expect(std::strcmp(row,
         "120200,valid,0,0,200,205,312,package_present,package_detected,none\n") == 0,
         "CSV includes timestamp, readings, state and event");

  record.quality = SampleQuality::RangeInvalid;
  record.rangeStatus = 2;
  record.apiError = -7;
  formatSampleCsvRow(row, sizeof(row), record);
  expect(std::strstr(row, ",range_invalid,-7,2,") != nullptr,
         "invalid returns retain their diagnostics");
  record.quality = SampleQuality::ManualMarker;
  record.marker = ExperimentMarker::ObjectPlaced;
  record.event = DetectorEvent::None;
  formatSampleCsvRow(row, sizeof(row), record);
  expect(std::strstr(row, ",manual_marker,") != nullptr &&
             std::strstr(row, ",none,object_placed\n") != nullptr,
         "manual action markers are exported beside sensor readings");
  std::cout << "Sample log tests passed.\n";
}
