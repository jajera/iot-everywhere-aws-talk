#ifndef BACKOFF_UTILS_H
#define BACKOFF_UTILS_H

#include <stdint.h>

namespace BackoffUtils {

inline uint32_t next(uint32_t current_ms, uint32_t initial_ms, uint32_t max_ms) {
  if (current_ms < initial_ms) {
    return initial_ms;
  }
  uint64_t doubled = static_cast<uint64_t>(current_ms) * 2ULL;
  if (doubled > max_ms) {
    return max_ms;
  }
  return static_cast<uint32_t>(doubled);
}

}  // namespace BackoffUtils

#endif  // BACKOFF_UTILS_H
