#pragma once

#include "hid_value.hpp"
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

namespace impl {
inline const std::vector<std::pair<std::string, consumer_key_code>>& get_consumer_key_code_name_value_pairs(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static std::vector<std::pair<std::string, consumer_key_code>> pairs({
      {"power", consumer_key_code::power},
      {"display_brightness_increment", consumer_key_code::display_brightness_increment},
      {"display_brightness_decrement", consumer_key_code::display_brightness_decrement},
      {"fastforward", consumer_key_code::fastforward},
      {"rewind", consumer_key_code::rewind},
      {"scan_next_track", consumer_key_code::scan_next_track},
      {"scan_previous_track", consumer_key_code::scan_previous_track},
      {"eject", consumer_key_code::eject},
      {"play_or_pause", consumer_key_code::play_or_pause},
      {"mute", consumer_key_code::mute},
      {"volume_increment", consumer_key_code::volume_increment},
      {"volume_decrement", consumer_key_code::volume_decrement},
  });

  return pairs;
}

inline const std::unordered_map<std::string, consumer_key_code>& get_consumer_key_code_name_value_map(void) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  static std::unordered_map<std::string, consumer_key_code> map;

  if (map.empty()) {
    for (const auto& pair : get_consumer_key_code_name_value_pairs()) {
      auto it = map.find(pair.first);
      if (it != std::end(map)) {
        logger::get_logger()->error("duplicate entry in get_consumer_key_code_name_value_pairs: {0}", pair.first);
      } else {
        map.emplace(pair.first, pair.second);
      }
    }
  }

  return map;
}
} // namespace impl

inline std::optional<consumer_key_code> make_consumer_key_code(const std::string& name) {
  auto& map = impl::get_consumer_key_code_name_value_map();
  auto it = map.find(name);
  if (it == map.end()) {
    return std::nullopt;
  }
  return it->second;
}

inline std::string make_consumer_key_code_name(consumer_key_code consumer_key_code) {
  for (const auto& pair : impl::get_consumer_key_code_name_value_pairs()) {
    if (pair.second == consumer_key_code) {
      return pair.first;
    }
  }
  return fmt::format("(number:{0})", static_cast<uint32_t>(consumer_key_code));
}

inline std::optional<consumer_key_code> make_consumer_key_code(hid_usage_page usage_page, hid_usage usage) {
  auto u = static_cast<uint32_t>(usage);

  switch (usage_page) {
    case hid_usage_page::consumer:
      switch (consumer_key_code(u)) {
        case consumer_key_code::power:
        case consumer_key_code::display_brightness_increment:
        case consumer_key_code::display_brightness_decrement:
        case consumer_key_code::fastforward:
        case consumer_key_code::rewind:
        case consumer_key_code::scan_next_track:
        case consumer_key_code::scan_previous_track:
        case consumer_key_code::eject:
        case consumer_key_code::play_or_pause:
        case consumer_key_code::mute:
        case consumer_key_code::volume_increment:
        case consumer_key_code::volume_decrement:
          return consumer_key_code(u);
      }

    default:
      break;
  }

  return std::nullopt;
}

inline std::optional<consumer_key_code> make_consumer_key_code(const hid_value& hid_value) {
  if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
    if (auto hid_usage = hid_value.get_hid_usage()) {
      return make_consumer_key_code(*hid_usage_page,
                                    *hid_usage);
    }
  }
  return std::nullopt;
}

inline std::optional<hid_usage_page> make_hid_usage_page(consumer_key_code consumer_key_code) {
  return hid_usage_page::consumer;
}

inline std::optional<hid_usage> make_hid_usage(consumer_key_code consumer_key_code) {
  return hid_usage(static_cast<uint32_t>(consumer_key_code));
}

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
