#pragma once

#include "detector_config.h"

inline DetectorConfig desktopTestConfig() {
  DetectorConfig config;
  config.detectionDeltaMm = 60;
  config.clearDeltaMm = 30;
  config.detectionSamples = 50;
  // Follow a farther empty background in a few seconds. The detector never
  // adapts toward closer readings, so a package cannot be learned away.
  config.baselineAlpha = 0.05f;
  return config;
}
