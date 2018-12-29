#pragma once

#include "manipulator/details/conditions/base.hpp"
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class variable final : public base {
public:
  enum class type {
    variable_if,
    variable_unless,
  };

  variable(const nlohmann::json& json) : base(),
                                         type_(type::variable_if) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "type") {
          if (value.is_string()) {
            if (value == "variable_if") {
              type_ = type::variable_if;
            }
            if (value == "variable_unless") {
              type_ = type::variable_unless;
            }
          }
        } else if (key == "name") {
          if (value.is_string()) {
            name_ = value;
          } else {
            logger::get_logger()->error("complex_modifications json error: Invalid form of {0} in {1}", key, json.dump());
          }
        } else if (key == "value") {
          if (value.is_number()) {
            value_ = value;
          } else {
            logger::get_logger()->error("complex_modifications json error: Invalid form of {0} in {1}", key, json.dump());
          }
        } else {
          logger::get_logger()->error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  virtual ~variable(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    switch (type_) {
      case type::variable_if:
        return manipulator_environment.get_variable(name_) == value_;
      case type::variable_unless:
        return manipulator_environment.get_variable(name_) != value_;
    }
  }

private:
  type type_;
  std::string name_;
  int value_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
