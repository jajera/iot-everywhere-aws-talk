#ifndef CONNECTIVITY_TIMEOUT_UTILS_H
#define CONNECTIVITY_TIMEOUT_UTILS_H

#include <stdint.h>

namespace ConnectivityTimeoutUtils {

inline bool shouldReset(uint32_t now_ms, uint32_t last_dual_connected_ms, uint32_t timeout_ms) {
  return (now_ms - last_dual_connected_ms) >= timeout_ms;
}

}  // namespace ConnectivityTimeoutUtils

#endif  // CONNECTIVITY_TIMEOUT_UTILS_H
