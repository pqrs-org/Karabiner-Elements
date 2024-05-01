#pragma once

#include "types.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class system final {
public:
  system(void) : json_(nlohmann::json::object()),
                               enable_notifications_(true) {
  }

  const nlohmann::json& get_json(void) const {
    return json_;
  }

  void set_json(const nlohmann::json& value) {
    json_ = value;
  }

  bool get_enable_notifications(void) const {
    return enable_notifications_;
  }

  void set_enable_notifications(bool value) {
    enable_notifications_ = value;
  }

  bool operator==(const system& other) const {
    // Skip `json_`.
    return enable_notifications_ == other.enable_notifications_;
  }

private:
  nlohmann::json json_;
  bool enable_notifications_;
};

inline void to_json(nlohmann::json& json, const system& value) {
  json = value.get_json();
  json["enable_notifications"] = value.get_enable_notifications();
}

inline void from_json(const nlohmann::json& json, system& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "enable_notifications") {
      pqrs::json::requires_boolean(v, "`" + k + "`");

      value.set_enable_notifications(v.get<int>());

    } else {
      // Allow unknown keys in order to be able to load
      // newer version of karabiner.json with older Karabiner-Elements.
    }
  }

  value.set_json(json);
}
} // namespace details
} // namespace core_configuration
} // namespace krbn

namespace std {
template <>
struct hash<krbn::core_configuration::details::system> final {
  std::size_t operator()(const krbn::core_configuration::details::system& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_enable_notifications());

    return h;
  }
};
} // namespace std
