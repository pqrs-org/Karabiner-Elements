#pragma once

#include "stream_utility.hpp"
#include <cstdint>
#include <nlohmann/json.hpp>

namespace krbn {
enum class event_type : uint32_t {
  key_down,
  key_up,
  single,
};

inline std::ostream& operator<<(std::ostream& stream, const event_type& value) {
  switch (value) {
    case event_type::key_down:
      stream << "key_down";
      break;
    case event_type::key_up:
      stream << "key_up";
      break;
    case event_type::single:
      stream << "single";
      break;
  }

  return stream;
}

inline void to_json(nlohmann::json& json, const event_type& value) {
  switch (value) {
    case event_type::key_down:
      json = "key_down";
      break;
    case event_type::key_up:
      json = "key_up";
      break;
    case event_type::single:
      json = "single";
      break;
  }
}

inline void from_json(const nlohmann::json& json, event_type& value) {
  auto s = json.get<std::string>();

  if (s == "key_up") {
    value = event_type::key_up;
  } else if (s == "single") {
    value = event_type::single;
  } else {
    value = event_type::key_down;
  }
}
} // namespace krbn
