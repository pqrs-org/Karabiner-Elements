#pragma once

#include <ostream>
#include <pqrs/json.hpp>
#include <spdlog/fmt/fmt.h>
#include <variant>

namespace krbn {
class manipulator_environment_variable_value final {
public:
  using value_t = std::variant<int,
                               bool,
                               std::string>;

  manipulator_environment_variable_value(void)
      : value_(0) {}

  manipulator_environment_variable_value(value_t value)
      : value_(value) {}

  const value_t& get_value(void) const {
    return value_;
  }

  template <typename T>
  const T* get_if(void) const {
    return std::get_if<T>(&value_);
  }

  bool operator==(const manipulator_environment_variable_value& other) const {
    return value_ == other.value_;
  }

private:
  value_t value_;
};

inline void to_json(nlohmann::json& j, const manipulator_environment_variable_value& value) {
  if (auto v = value.get_if<int>()) {
    j = *v;
  } else if (auto v = value.get_if<bool>()) {
    j = *v;
  } else if (auto v = value.get_if<std::string>()) {
    j = *v;
  }
}

inline void from_json(const nlohmann::json& j, manipulator_environment_variable_value& value) {
  if (j.is_number()) {
    value = manipulator_environment_variable_value(j.get<int>());
  } else if (j.is_boolean()) {
    value = manipulator_environment_variable_value(j.get<bool>());
  } else if (j.is_string()) {
    value = manipulator_environment_variable_value(j.get<std::string>());
  } else {
    throw pqrs::json::unmarshal_error(
        fmt::format("json must be number, boolean or string, but is `{0}`",
                    pqrs::json::dump_for_error_message(j)));
  }
}

inline std::ostream& operator<<(std::ostream& stream, const manipulator_environment_variable_value& value) {
  stream << nlohmann::json(value);
  return stream;
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::manipulator_environment_variable_value> final {
  std::size_t operator()(const krbn::manipulator_environment_variable_value& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_value());

    return h;
  }
};
} // namespace std
