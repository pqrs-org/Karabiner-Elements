#pragma once

#include "manipulator/details/conditions/base.hpp"
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class keyboard_type final : public base {
public:
  enum class type {
    keyboard_type_if,
    keyboard_type_unless,
  };

  keyboard_type(const nlohmann::json& json) : base(),
                                              type_(type::keyboard_type_if) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "type") {
          if (value.is_string()) {
            if (value == "keyboard_type_if") {
              type_ = type::keyboard_type_if;
            }
            if (value == "keyboard_type_unless") {
              type_ = type::keyboard_type_unless;
            }
          }
        } else if (key == "keyboard_types") {
          for (const auto& j : value) {
            keyboard_types_.push_back(j.get<std::string>());
          }
        } else {
          logger::get_logger()->error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  virtual ~keyboard_type(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    for (const auto& t : keyboard_types_) {
      if (t == manipulator_environment.get_keyboard_type()) {
        switch (type_) {
          case type::keyboard_type_if:
            return true;
          case type::keyboard_type_unless:
            return false;
        }
      }
    }

    // Not found

    switch (type_) {
      case type::keyboard_type_if:
        return false;
      case type::keyboard_type_unless:
        return true;
    }

    return false;
  }

private:
  type type_;
  std::vector<std::string> keyboard_types_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
