#pragma once

#include <stdint.h>

struct DetectorConfig {
  // Detection threshold / 检测阈值：距离至少缩短这么多，才可能是快递。
  uint16_t detectionDeltaMm = 120;

  // Hysteresis / 迟滞：使用更小的清除阈值，避免状态在边界反复跳动。
  uint16_t clearDeltaMm = 60;

  // Debounce / 连续采样确认：一次异常读数不会立刻触发事件。
  uint8_t detectionSamples = 8;
  uint8_t clearSamples = 12;

  // Startup calibration / 启动校准：用空门口的数据建立基准距离。
  uint8_t calibrationSamples = 30;

  // EMA weights / 指数移动平均权重：越小越平滑，但反应也越慢。
  float measurementAlpha = 0.25f;
  float baselineAlpha = 0.01f;
};
