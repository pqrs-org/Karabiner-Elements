#pragma once

#include "stream_utility.hpp"
#include <cstdint>

namespace krbn {
enum class consumer_key_code : uint32_t {
  power = kHIDUsage_Csmr_Power,
  display_brightness_increment = kHIDUsage_Csmr_DisplayBrightnessIncrement,
  display_brightness_decrement = kHIDUsage_Csmr_DisplayBrightnessDecrement,
  fastforward = kHIDUsage_Csmr_FastForward,
  rewind = kHIDUsage_Csmr_Rewind,
  scan_next_track = kHIDUsage_Csmr_ScanNextTrack,
  scan_previous_track = kHIDUsage_Csmr_ScanPreviousTrack,
  eject = kHIDUsage_Csmr_Eject,
  play_or_pause = kHIDUsage_Csmr_PlayOrPause,
  mute = kHIDUsage_Csmr_Mute,
  volume_increment = kHIDUsage_Csmr_VolumeIncrement,
  volume_decrement = kHIDUsage_Csmr_VolumeDecrement,
};

inline std::ostream& operator<<(std::ostream& stream, const consumer_key_code& value) {
  return stream_utility::output_enum(stream, value);
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<consumer_key_code, std::allocator<consumer_key_code>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<consumer_key_code, std::hash<consumer_key_code>, std::equal_to<consumer_key_code>, std::allocator<consumer_key_code>>& values) {
  return stream_utility::output_enums(stream, values);
}
} // namespace krbn
