#pragma once

#include "base.hpp"
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace conditions {
class variable final : public base {
public:
  enum class type {
    variable_if,
    variable_unless,
  };

  variable(const nlohmann::json& json) : base(),
                                         type_(type::variable_if) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("{0} must be string, but is `{1}`", key, value.dump()));
        }

        auto t = value.get<std::string>();

        if (t == "variable_if") {
          type_ = type::variable_if;
        } else if (t == "variable_unless") {
          type_ = type::variable_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "name") {
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be string, but is `{1}`", key, value.dump()));
        }

        name_ = value.get<std::string>();

      } else if (key == "value") {
        if (!value.is_number()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be number, but is `{1}`", key, value.dump()));
        }

        value_ = value.get<int>();

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
      }
    }

    if (!name_) {
      throw pqrs::json::unmarshal_error(fmt::format("`name` is not found in `{0}`", json.dump()));
    }

    if (!value_) {
      throw pqrs::json::unmarshal_error(fmt::format("`value` is not found in `{0}`", json.dump()));
    }
  }

  virtual ~variable(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    switch (type_) {
      case type::variable_if:
        return manipulator_environment.get_variable(*name_) == *value_;
      case type::variable_unless:
        return manipulator_environment.get_variable(*name_) != *value_;
    }
  }

private:
  type type_;
  std::optional<std::string> name_;
  std::optional<int> value_;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
