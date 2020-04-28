#pragma once

#include "modifier_definition.hpp"
#include <pqrs/json.hpp>
#include <set>
#include <unordered_set>

namespace krbn {
namespace manipulator {
class from_modifiers_definition final {
public:
  from_modifiers_definition(void) {
  }

  virtual ~from_modifiers_definition(void) {
  }

  const std::set<modifier_definition::modifier>& get_mandatory_modifiers(void) const {
    return mandatory_modifiers_;
  }

  void set_mandatory_modifiers(const std::set<modifier_definition::modifier>& value) {
    mandatory_modifiers_ = value;
  }

  const std::set<modifier_definition::modifier>& get_optional_modifiers(void) const {
    return optional_modifiers_;
  }

  void set_optional_modifiers(const std::set<modifier_definition::modifier>& value) {
    optional_modifiers_ = value;
  }

  std::shared_ptr<std::unordered_set<modifier_flag>> test_modifiers(const modifier_flag_manager& modifier_flag_manager) const {
    auto modifier_flags = std::make_shared<std::unordered_set<modifier_flag>>();

    // If mandatory_modifiers_ contains modifier::any, return all active modifier_flags.

    if (mandatory_modifiers_.find(modifier_definition::modifier::any) != std::end(mandatory_modifiers_)) {
      for (auto i = static_cast<uint32_t>(modifier_flag::zero) + 1; i != static_cast<uint32_t>(modifier_flag::end_); ++i) {
        auto flag = modifier_flag(i);
        if (modifier_flag_manager.is_pressed(flag)) {
          modifier_flags->insert(flag);
        }
      }
      return modifier_flags;
    }

    // Check modifier_flag state.

    for (int i = 0; i < static_cast<int>(modifier_definition::modifier::end_); ++i) {
      auto m = modifier_definition::modifier(i);

      if (mandatory_modifiers_.find(m) != std::end(mandatory_modifiers_)) {
        auto pair = test_modifier(modifier_flag_manager, m);
        if (!pair.first) {
          return nullptr;
        }
        if (pair.second != modifier_flag::zero) {
          modifier_flags->insert(pair.second);
        }
      }
    }

    // If optional_modifiers_ does not contain modifier::any, we have to check modifier flags strictly.

    if (optional_modifiers_.find(modifier_definition::modifier::any) == std::end(optional_modifiers_)) {
      std::unordered_set<modifier_flag> extra_modifier_flags;
      for (auto m = static_cast<uint32_t>(modifier_flag::zero) + 1; m != static_cast<uint32_t>(modifier_flag::end_); ++m) {
        extra_modifier_flags.insert(modifier_flag(m));
      }

      for (int i = 0; i < static_cast<int>(modifier_definition::modifier::end_); ++i) {
        auto m = modifier_definition::modifier(i);

        if (mandatory_modifiers_.find(m) != std::end(mandatory_modifiers_) ||
            optional_modifiers_.find(m) != std::end(optional_modifiers_)) {
          for (const auto& flag : modifier_definition::get_modifier_flags(m)) {
            extra_modifier_flags.erase(flag);
          }
        }
      }

      for (const auto& flag : extra_modifier_flags) {
        if (modifier_flag_manager.is_pressed(flag)) {
          return nullptr;
        }
      }
    }

    return modifier_flags;
  }

  static std::pair<bool, modifier_flag> test_modifier(const modifier_flag_manager& modifier_flag_manager,
                                                      modifier_definition::modifier modifier) {
    if (modifier == modifier_definition::modifier::any) {
      return std::make_pair(true, modifier_flag::zero);
    }

    auto modifier_flags = modifier_definition::get_modifier_flags(modifier);
    if (!modifier_flags.empty()) {
      for (const auto& m : modifier_flags) {
        if (modifier_flag_manager.is_pressed(m)) {
          return std::make_pair(true, m);
        }
      }
    }

    return std::make_pair(false, modifier_flag::zero);
  }

private:
  std::set<modifier_definition::modifier> mandatory_modifiers_;
  std::set<modifier_definition::modifier> optional_modifiers_;
};

inline void from_json(const nlohmann::json& json, from_modifiers_definition& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "mandatory") {
      try {
        value.set_mandatory_modifiers(modifier_definition::make_modifiers(v));
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", k, e.what()));
      }

    } else if (k == "optional") {
      try {
        value.set_optional_modifiers(modifier_definition::make_modifiers(v));
      } catch (const pqrs::json::unmarshal_error& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` error: {1}", k, e.what()));
      }

    } else if (k == "description") {
      // Do nothing

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", k, pqrs::json::dump_for_error_message(json)));
    }
  }
}
} // namespace manipulator
} // namespace krbn
