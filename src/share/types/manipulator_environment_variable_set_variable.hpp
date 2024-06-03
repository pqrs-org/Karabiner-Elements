#pragma once

#include "manipulator_environment_variable_value.hpp"

namespace krbn {
class manipulator_environment_variable_set_variable final {
public:
  enum class type {
    set,
    unset,
  };

  manipulator_environment_variable_set_variable(void)
      : type_(manipulator_environment_variable_set_variable::type::set) {
  }

  manipulator_environment_variable_set_variable(std::optional<std::string> name,
                                                std::optional<manipulator_environment_variable_value> value,
                                                std::optional<manipulator_environment_variable_value> key_up_value,
                                                type type = type::set)
      : name_(name),
        value_(value),
        key_up_value_(key_up_value),
        type_(type) {
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

  type get_type(void) const {
    return type_;
  }

  void set_type(type value) {
    type_ = value;
  }

  bool operator==(const manipulator_environment_variable_set_variable& other) const {
    return name_ == other.name_ &&
           value_ == other.value_ &&
           key_up_value_ == other.key_up_value_ &&
           type_ == other.type_;
  }

  bool operator!=(const manipulator_environment_variable_set_variable& other) const {
    return !(*this == other);
  }

private:
  std::optional<std::string> name_;
  std::optional<manipulator_environment_variable_value> value_;
  std::optional<manipulator_environment_variable_value> key_up_value_;
  type type_;
};

inline void to_json(nlohmann::json& json, const manipulator_environment_variable_set_variable& m) {
  json = nlohmann::json::object();

  if (auto v = m.get_name()) {
    json["name"] = *v;
  }

  if (auto v = m.get_value()) {
    json["value"] = *v;
  }

  if (auto v = m.get_key_up_value()) {
    json["key_up_value"] = *v;
  }

  switch (m.get_type()) {
    case manipulator_environment_variable_set_variable::type::set:
      json["type"] = "set";
      break;
    case manipulator_environment_variable_set_variable::type::unset:
      json["type"] = "unset";
      break;
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

    } else if (key == "type") {
      pqrs::json::requires_string(value, "`" + key + "`");

      auto v = value.get<std::string>();
      if (v == "set") {
        m.set_type(manipulator_environment_variable_set_variable::type::set);
      } else if (v == "unset") {
        m.set_type(manipulator_environment_variable_set_variable::type::unset);
      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown type: `{0}`", v));
      }

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
    pqrs::hash::combine(h, value.get_key_up_value());
    pqrs::hash::combine(h, value.get_type());

    return h;
  }
};
} // namespace std
