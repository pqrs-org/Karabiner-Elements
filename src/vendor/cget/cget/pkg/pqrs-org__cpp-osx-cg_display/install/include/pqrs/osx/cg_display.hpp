#pragma once

// pqrs::osx::cg_display v1.0

// (C) Copyright Takayama Fumihiko 2021.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#include <CoreGraphics/CoreGraphics.h>
#include <vector>

namespace pqrs {
namespace osx {
namespace cg_display {

inline uint32_t online_display_count(void) {
  uint32_t display_count = 0;
  CGGetOnlineDisplayList(0, nullptr, &display_count);
  return display_count;
}

inline std::vector<CGDirectDisplayID> online_displays(void) {
  auto display_count = online_display_count();

  std::vector<CGDirectDisplayID> displays(display_count);
  CGGetOnlineDisplayList(displays.size(), displays.data(), &display_count);
  return displays;
}

inline uint32_t active_display_count(void) {
  uint32_t display_count = 0;
  CGGetActiveDisplayList(0, nullptr, &display_count);
  return display_count;
}

inline std::vector<CGDirectDisplayID> active_displays(void) {
  auto display_count = active_display_count();

  std::vector<CGDirectDisplayID> displays(display_count);
  CGGetActiveDisplayList(displays.size(), displays.data(), &display_count);
  return displays;
}

} // namespace cg_display
} // namespace osx
} // namespace pqrs
