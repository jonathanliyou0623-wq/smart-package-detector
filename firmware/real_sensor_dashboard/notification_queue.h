#pragma once

#include <cstddef>
#include <cstdint>

enum class NotificationKind : uint8_t {
  PackageDetected,
  PackageRemoved,
};

struct NotificationRecord {
  uint32_t sequence = 0;
  uint64_t uptimeMs = 0;
  uint16_t rawMm = 0;
  uint16_t filteredMm = 0;
  uint16_t baselineMm = 0;
  uint8_t attempts = 0;
  NotificationKind kind = NotificationKind::PackageDetected;
};

// A bounded FIFO keeps detector events while Wi-Fi or the broker is unavailable.
// The oldest event is preserved when full; newer events are rejected and counted.
class NotificationQueue {
 public:
  static constexpr size_t kCapacity = 8;

  bool enqueue(const NotificationRecord& record) {
    if (count_ == kCapacity) {
      ++dropped_;
      return false;
    }
    records_[(head_ + count_) % kCapacity] = record;
    ++count_;
    return true;
  }

  bool empty() const { return count_ == 0; }
  size_t size() const { return count_; }
  uint32_t dropped() const { return dropped_; }

  const NotificationRecord& front() const { return records_[head_]; }

  bool markAttempt(uint32_t sequence) {
    if (empty() || records_[head_].sequence != sequence) return false;
    if (records_[head_].attempts < UINT8_MAX) ++records_[head_].attempts;
    return true;
  }

  bool popIfSequence(uint32_t sequence) {
    if (empty() || records_[head_].sequence != sequence) return false;
    head_ = (head_ + 1) % kCapacity;
    --count_;
    return true;
  }

 private:
  NotificationRecord records_[kCapacity]{};
  size_t head_ = 0;
  size_t count_ = 0;
  uint32_t dropped_ = 0;
};

inline const char* notificationKindName(NotificationKind kind) {
  switch (kind) {
    case NotificationKind::PackageDetected: return "package_detected";
    case NotificationKind::PackageRemoved: return "package_removed";
  }
  return "unknown";
}
