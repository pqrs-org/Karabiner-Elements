#pragma once

#include <chrono>

namespace krbn {
namespace chrono_utility {
inline std::size_t milliseconds_since_epoch(void) {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

inline std::size_t nanoseconds_since_epoch(void) {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}
}; // namespace chrono_utility
} // namespace krbn
