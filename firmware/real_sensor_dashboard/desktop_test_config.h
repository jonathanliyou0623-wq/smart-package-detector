#pragma once

#include "detector_config.h"

inline DetectorConfig desktopTestConfig() {
  DetectorConfig config;
  config.detectionDeltaMm = 60;
  config.clearDeltaMm = 30;
  config.detectionSamples = 50;
  config.baselineAlpha = 0.0f;
  return config;
}
