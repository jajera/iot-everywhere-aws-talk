#ifndef DEBOUNCE_UTILS_H
#define DEBOUNCE_UTILS_H

#include <stdint.h>

namespace DebounceUtils {

inline bool shouldAcceptPress(uint32_t now_ms, uint32_t last_accepted_ms, uint32_t min_gap_ms) {
  return (now_ms - last_accepted_ms) >= min_gap_ms;
}

}  // namespace DebounceUtils

#endif  // DEBOUNCE_UTILS_H
