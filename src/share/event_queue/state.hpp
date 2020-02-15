#pragma once

#include <pqrs/json.hpp>
#include <spdlog/fmt/fmt.h>

namespace krbn {
namespace event_queue {
enum class state {
  original,
  manipulated,
};

inline void to_json(nlohmann::json& json, const state& value) {
  switch (value) {
    case state::original:
      json = "original";
      break;
    case state::manipulated:
      json = "manipulated";
      break;
  }
}

inline void from_json(const nlohmann::json& json, state& value) {
  pqrs::json::requires_string(json, "json");

  if (json == "original") {
    value = state::original;
  } else if (json == "manipulated") {
    value = state::manipulated;
  } else {
    throw pqrs::json::unmarshal_error(fmt::format("unknown state: `{0}`", json.get<std::string>()));
  }
}
} // namespace event_queue
} // namespace krbn
