#ifndef INTERVAL_UTILS_H
#define INTERVAL_UTILS_H

#include <stdint.h>

namespace IntervalUtils {

inline bool shouldPublish(uint32_t now_ms, uint32_t last_publish_ms, uint32_t interval_sec) {
  const uint32_t target_ms = interval_sec * 1000UL;
  return (now_ms - last_publish_ms) >= target_ms;
}

inline bool withinTolerance(uint32_t elapsed_ms, uint32_t interval_sec, uint32_t tolerance_sec) {
  const int64_t target = static_cast<int64_t>(interval_sec) * 1000LL;
  const int64_t tolerance = static_cast<int64_t>(tolerance_sec) * 1000LL;
  const int64_t elapsed = static_cast<int64_t>(elapsed_ms);
  return elapsed >= (target - tolerance) && elapsed <= (target + tolerance);
}

}  // namespace IntervalUtils

#endif  // INTERVAL_UTILS_H
