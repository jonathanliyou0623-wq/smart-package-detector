#pragma once

#include "detector_config.h"

// Controlled tabletop experiment: ~311 mm background, ~200 mm tissue roll.
// Recalibrate with the scene empty whenever the sensor or background moves.
inline DetectorConfig desktopTestConfig() {
  DetectorConfig config;
  config.detectionDeltaMm = 60;
  config.clearDeltaMm = 30;
  config.baselineAlpha = 0.0f;
  return config;
}
