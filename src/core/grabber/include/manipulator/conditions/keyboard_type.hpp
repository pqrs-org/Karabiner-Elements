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
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("keyboard_type must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("{0} must be string, but is `{1}`", key, value.dump()));
        }

        auto t = value.get<std::string>();

        if (t == "keyboard_type_if") {
          type_ = type::keyboard_type_if;
        } else if (t == "keyboard_type_unless") {
          type_ = type::keyboard_type_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "keyboard_types") {
        if (!value.is_array()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
        }

        for (const auto& j : value) {
          if (!j.is_string()) {
            throw pqrs::json::unmarshal_error(fmt::format("keyboard_types entry must be string, but is `{0}`", j.dump()));
          }

          keyboard_types_.push_back(j.get<std::string>());
        }

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
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
} // namespace manipulator
} // namespace krbn
