#pragma once

#include "manipulator/details/conditions/base.hpp"
#include <regex>
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
          handle_input_sources(value);
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
  void handle_input_sources(const nlohmann::json& json) {
    for (const auto& j : json) {
      if (j.is_object()) {
        boost::optional<std::regex> language_regex;
        boost::optional<std::regex> input_source_id_regex;
        boost::optional<std::regex> input_mode_id_regex;

        for (auto it = std::begin(j); it != std::end(j); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "language") {
            if (value.is_string()) {
              language_regex = std::regex(value.get<std::string>());
            } else {
              logger::get_logger().error("complex_modifications json error: input_sources.language should be string: {0}", json.dump());
            }
          } else if (key == "input_source_id") {
            if (value.is_string()) {
              input_source_id_regex = std::regex(value.get<std::string>());
            } else {
              logger::get_logger().error("complex_modifications json error: input_sources.input_source_id should be string: {0}", json.dump());
            }
          } else if (key == "input_mode_id") {
            if (value.is_string()) {
              input_mode_id_regex = std::regex(value.get<std::string>());
            } else {
              logger::get_logger().error("complex_modifications json error: input_sources.input_mode_id should be string: {0}", json.dump());
            }
          } else {
            logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
          }
        }

        input_source_selectors_.emplace_back(language_regex,
                                             input_source_id_regex,
                                             input_mode_id_regex);

      } else {
        logger::get_logger().error("complex_modifications json error: input_sources should be array of object: {0}", json.dump());
      }
    }
  }

  type type_;
  std::vector<input_source_selector> input_source_selectors_;

  mutable boost::optional<std::pair<input_source_identifiers, bool>> cached_result_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
