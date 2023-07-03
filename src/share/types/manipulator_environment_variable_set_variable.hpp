#pragma once

#include "manipulator_environment_variable_value.hpp"

namespace krbn {
class manipulator_environment_variable_set_variable final {
public:
  manipulator_environment_variable_set_variable(void) {
  }

  manipulator_environment_variable_set_variable(std::optional<std::string> name,
                                                std::optional<manipulator_environment_variable_value> value,
                                                std::optional<manipulator_environment_variable_value> key_up_value)
      : name_(name),
        value_(value),
        key_up_value_(key_up_value) {
  }

  std::optional<std::string> get_name(void) const {
    return name_;
  }

  void set_name(std::optional<std::string> value) {
    name_ = value;
  }

  std::optional<manipulator_environment_variable_value> get_value(void) const {
    return value_;
  }

  void set_value(std::optional<manipulator_environment_variable_value> value) {
    value_ = value;
  }

  std::optional<manipulator_environment_variable_value> get_key_up_value(void) const {
    return key_up_value_;
  }

  void set_key_up_value(std::optional<manipulator_environment_variable_value> value) {
    key_up_value_ = value;
  }

private:
  std::optional<std::string> name_;
  std::optional<manipulator_environment_variable_value> value_;
  std::optional<manipulator_environment_variable_value> key_up_value_;
};

inline void to_json(nlohmann::json& json, const manipulator_environment_variable_set_variable& m) {
  if (auto v = m.get_name()) {
    json["name"] = *v;
  }

  if (auto v = m.get_value()) {
    json["value"] = *v;
  }

  if (auto v = m.get_key_up_value()) {
    json["key_up_value"] = *v;
  }
}

inline void from_json(const nlohmann::json& json, manipulator_environment_variable_set_variable& m) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [key, value] : json.items()) {
    if (key == "name") {
      pqrs::json::requires_string(value, "`" + key + "`");

      m.set_name(value.get<std::string>());

    } else if (key == "value") {
      m.set_value(value.get<manipulator_environment_variable_value>());

    } else if (key == "key_up_value") {
      m.set_key_up_value(value.get<manipulator_environment_variable_value>());

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", key));
    }
  }
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::manipulator_environment_variable_set_variable> final {
  std::size_t operator()(const krbn::manipulator_environment_variable_set_variable& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_name());
    pqrs::hash::combine(h, value.get_value());
    pqrs::hash::combine(h, value.get_value());

    return h;
  }
};
} // namespace std
