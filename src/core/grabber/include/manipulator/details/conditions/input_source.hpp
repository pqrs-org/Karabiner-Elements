#pragma once

#include "manipulator/details/conditions/base.hpp"
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class input_source final : public base {
public:
  enum class type {
    input_source_if,
    input_source_unless,
  };

  input_source(const nlohmann::json& json) : base(),
                                             type_(type::input_source_if) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "type") {
          if (value.is_string()) {
            if (value == "input_source_if") {
              type_ = type::input_source_if;
            }
            if (value == "input_source_unless") {
              type_ = type::input_source_unless;
            }
          }
        } else if (key == "input_sources") {
          for (const auto& j : value) {
            input_source_selectors_.emplace_back(j);
          }
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  virtual ~input_source(void) {
  }

  virtual bool is_fulfilled(const event_queue::queued_event& queued_event,
                            const manipulator_environment& manipulator_environment) const {
    if (cached_result_ && cached_result_->first == manipulator_environment.get_input_source_identifiers()) {
      return cached_result_->second;
    }

    bool result = false;

    for (const auto& s : input_source_selectors_) {
      if (s.test(manipulator_environment.get_input_source_identifiers())) {
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
    cached_result_ = std::make_pair(manipulator_environment.get_input_source_identifiers(), result);
    return result;
  }

private:
  type type_;
  std::vector<input_source_selector> input_source_selectors_;

  mutable boost::optional<std::pair<input_source_identifiers, bool>> cached_result_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
