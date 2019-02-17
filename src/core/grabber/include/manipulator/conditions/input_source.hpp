#pragma once

#include "base.hpp"
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace conditions {
class input_source final : public base {
public:
  enum class type {
    input_source_if,
    input_source_unless,
  };

  input_source(const nlohmann::json& json) : base(),
                                             type_(type::input_source_if) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("input_source must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("{0} must be string, but is `{1}`", key, value.dump()));
        }

        auto t = value.get<std::string>();

        if (t == "input_source_if") {
          type_ = type::input_source_if;
        } else if (t == "input_source_unless") {
          type_ = type::input_source_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "input_sources") {
        if (!value.is_array()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
        }

        for (const auto& j : value) {
          try {
            input_source_specifiers_.emplace_back(j);
          } catch (pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
          }
        }

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
      }
    }
  }

  virtual ~input_source(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    if (cached_result_ && cached_result_->first == manipulator_environment.get_input_source_properties()) {
      return cached_result_->second;
    }

    bool result = false;

    for (const auto& s : input_source_specifiers_) {
      if (s.test(manipulator_environment.get_input_source_properties())) {
        switch (type_) {
          case type::input_source_if:
            result = true;
            goto finish;
          case type::input_source_unless:
            result = false;
            goto finish;
        }
      }
    }

    // Not found

    switch (type_) {
      case type::input_source_if:
        result = false;
        goto finish;
      case type::input_source_unless:
        result = true;
        goto finish;
    }

  finish:
    cached_result_ = std::make_pair(manipulator_environment.get_input_source_properties(), result);
    return result;
  }

private:
  type type_;
  std::vector<pqrs::osx::input_source_selector::specifier> input_source_specifiers_;

  mutable std::optional<std::pair<pqrs::osx::input_source::properties, bool>> cached_result_;
};
} // namespace conditions
} // namespace manipulator
} // namespace krbn
