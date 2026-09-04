#pragma once

#include <stdint.h>

struct DetectorConfig {
  // A package must reduce the measured distance by at least this amount.
  uint16_t detectionDeltaMm = 120;

  // Hysteresis: the reading is considered clear below this delta.
  uint16_t clearDeltaMm = 60;

  // Number of consecutive samples required to confirm each transition.
  uint8_t detectionSamples = 8;
  uint8_t clearSamples = 12;

  // Number of valid readings used to establish the startup baseline.
  uint8_t calibrationSamples = 30;

  // Filtering weights in [0, 1]. Lower values react more slowly.
  float measurementAlpha = 0.25f;
  float baselineAlpha = 0.01f;
};
