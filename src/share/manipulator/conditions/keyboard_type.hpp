#pragma once

#include "base.hpp"
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace conditions {
class keyboard_type final : public base {
public:
  enum class type {
    keyboard_type_if,
    keyboard_type_unless,
  };

  keyboard_type(const nlohmann::json& json) : base(),
                                              type_(type::keyboard_type_if) {
    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        pqrs::json::requires_string(value, key);

        auto t = value.get<std::string>();

        if (t == "keyboard_type_if") {
          type_ = type::keyboard_type_if;
        } else if (t == "keyboard_type_unless") {
          type_ = type::keyboard_type_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "keyboard_types") {
        pqrs::json::requires_array(value, "`keyboard_types`");

        for (const auto& j : value) {
          pqrs::json::requires_string(j, "keyboard_types entry");

          keyboard_types_.push_back(j.get<std::string>());
        }

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, pqrs::json::dump_for_error_message(json)));
      }
    }
  }

  virtual ~keyboard_type(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    for (const auto& t : keyboard_types_) {
      if (t == manipulator_environment.get_virtual_hid_keyboard_keyboard_type()) {
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
} // namespace manipulator
} // namespace krbn
