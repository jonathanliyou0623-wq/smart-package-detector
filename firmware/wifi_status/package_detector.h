#pragma once

#include <cmath>
#include <cstdint>

#include "detector_config.h"

enum class DetectorState : uint8_t {
  Calibrating,   // Learning the empty-door baseline / 学习空门口基准距离
  Clear,         // No persistent obstruction / 没有持续遮挡
  PackagePresent,  // A persistent object has been confirmed / 已确认有物体
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
    // Zero is treated as invalid input / 0 被视为无效测距，不改变状态。
    if (distanceMm == 0) {
      return DetectorEvent::None;
    }

    if (state_ == DetectorState::Calibrating) {
      // Average the first valid samples / 对启动阶段的有效样本取平均值。
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
      // Exponential moving average (EMA) / 指数移动平均，减小随机抖动。
      filteredMm_ += config_.measurementAlpha *
                     (static_cast<float>(distanceMm) - filteredMm_);
    }

    // Positive value means something is closer than the learned background.
    // 正值表示当前物体比学习到的背景更靠近传感器。
    const float obstructionMm = baselineMm_ - filteredMm_;

    if (state_ == DetectorState::Clear) {
      if (obstructionMm >= config_.detectionDeltaMm) {
        // Require multiple samples / 连续满足条件后才确认，过滤路人等短暂遮挡。
        ++transitionCount_;
        if (transitionCount_ >= config_.detectionSamples) {
          transitionCount_ = 0;
          state_ = DetectorState::PackagePresent;
          return DetectorEvent::PackageDetected;
        }
      } else {
        transitionCount_ = 0;
        // Track slow environmental drift only while clear.
        // 只在无快递状态下缓慢更新基准，避免把快递学习成背景。
        baselineMm_ += config_.baselineAlpha * (filteredMm_ - baselineMm_);
      }
    } else {
      if (obstructionMm <= config_.clearDeltaMm) {
        // Removal also needs confirmation / 快递移除也需要连续样本确认。
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
