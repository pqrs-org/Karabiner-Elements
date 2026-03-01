#pragma once

#include <nlohmann/json.hpp>
#include <ostream>

namespace krbn {
enum class event_input_source_backend {
  hid,
  cgeventtap,
};

inline std::ostream& operator<<(std::ostream& stream, const event_input_source_backend& value) {
  switch (value) {
    case event_input_source_backend::hid:
      stream << "hid";
      break;
    case event_input_source_backend::cgeventtap:
      stream << "cgeventtap";
      break;
  }

  return stream;
}

inline void to_json(nlohmann::json& json, const event_input_source_backend& value) {
  switch (value) {
    case event_input_source_backend::hid:
      json = "hid";
      break;
    case event_input_source_backend::cgeventtap:
      json = "cgeventtap";
      break;
  }
}

inline void from_json(const nlohmann::json& json, event_input_source_backend& value) {
  auto s = json.get<std::string>();

  if (s == "cgeventtap") {
    value = event_input_source_backend::cgeventtap;
  } else {
    value = event_input_source_backend::hid;
  }
}
} // namespace krbn
