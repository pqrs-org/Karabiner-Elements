#pragma once

#include "base.hpp"
#include <optional>
#include <string>

namespace krbn {
namespace manipulator {
namespace conditions {
class event_changed final : public base {
public:
  enum class type {
    event_changed_if,
    event_changed_unless,
  };

  event_changed(const nlohmann::json& json) : base(),
                                              type_(type::event_changed_if) {
    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        pqrs::json::requires_string(value, key);

        auto t = value.get<std::string>();

        if (t == "event_changed_if") {
          type_ = type::event_changed_if;
        } else if (t == "event_changed_unless") {
          type_ = type::event_changed_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "value") {
        pqrs::json::requires_boolean(value, "`value`");

        value_ = value.get<int>();

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
      }
    }

    if (!value_) {
      throw pqrs::json::unmarshal_error(fmt::format("`value` is not found in `{0}`", json.dump()));
    }
  }

  virtual ~event_changed(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    switch (type_) {
      case type::event_changed_if:
        if (value_) {
          return entry.get_state() == event_queue::state::manipulated;
        } else {
          return entry.get_state() != event_queue::state::manipulated;
        }
      case type::event_changed_unless:
        if (value_) {
          return entry.get_state() == event_queue::state::original;
        } else {
          return entry.get_state() != event_queue::state::original;
        }
    }

    return false;
  }

private:
  type type_;
  std::optional<bool> value_;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
