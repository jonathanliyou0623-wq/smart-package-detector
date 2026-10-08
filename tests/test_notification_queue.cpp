#include <cstdlib>
#include <iostream>
#include <string>

#include "notification_queue.h"

void expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(1);
  }
}

int main() {
  NotificationQueue queue;
  expect(queue.empty(), "queue starts empty");

  NotificationRecord record;
  for (size_t i = 0; i < NotificationQueue::kCapacity; ++i) {
    record.sequence = static_cast<uint32_t>(i + 1);
    record.uptimeMs = 1000 + i * 100;
    record.kind = i % 2 == 0 ? NotificationKind::PackageDetected
                             : NotificationKind::PackageRemoved;
    expect(queue.enqueue(record), "events fit up to the fixed capacity");
  }
  expect(queue.size() == NotificationQueue::kCapacity, "queue reaches capacity");

  record.sequence = 99;
  expect(!queue.enqueue(record), "full queue rejects a newer event");
  expect(queue.dropped() == 1, "rejected events are counted");
  expect(queue.front().sequence == 1, "overflow preserves the oldest event");

  expect(queue.markAttempt(1), "retry count updates the current event");
  expect(queue.front().attempts == 1, "retry count is retained");
  expect(!queue.markAttempt(2), "stale worker cannot modify another event");
  expect(!queue.popIfSequence(2), "stale acknowledgement cannot drop an event");

  for (uint32_t sequence = 1; sequence <= NotificationQueue::kCapacity; ++sequence) {
    expect(queue.front().sequence == sequence, "events remain FIFO ordered");
    expect(queue.popIfSequence(sequence), "matching acknowledgement removes event");
  }
  expect(queue.empty(), "queue drains after acknowledgements");
  expect(std::string(notificationKindName(NotificationKind::PackageDetected)) ==
             "package_detected",
         "arrival event has stable wire name");
  expect(std::string(notificationKindName(NotificationKind::PackageRemoved)) ==
             "package_removed",
         "removal event has stable wire name");

  std::cout << "Notification queue tests passed.\n";
}
