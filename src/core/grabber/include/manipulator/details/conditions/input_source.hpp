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
          if (value.is_array()) {
            for (const auto& j : value) {
              entries_.emplace_back(j);
            }
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
    if (cached_result_ && cached_result_->first == manipulator_environment.get_input_source()) {
      return cached_result_->second;
    }

    bool result = false;

    for (const auto& e : entries_) {
      if (e.is_matched(manipulator_environment.get_input_source())) {
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
    cached_result_ = std::make_pair(manipulator_environment.get_input_source(), result);
    return result;
  }

private:
  class entry final {
  public:
    entry(const nlohmann::json& json) {
      if (json.is_object()) {
        for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (key == "language") {
            if (value.is_string()) {
              language_ = std::regex(value.get<std::string>());
            } else {
              logger::get_logger().error("complex_modifications json error: input_sources.language should be string: {0}", json.dump());
            }
          } else if (key == "input_source_id") {
            if (value.is_string()) {
              input_source_id_ = std::regex(value.get<std::string>());
            } else {
              logger::get_logger().error("complex_modifications json error: input_sources.input_source_id should be string: {0}", json.dump());
            }
          } else if (key == "input_mode_id") {
            if (value.is_string()) {
              input_mode_id_ = std::regex(value.get<std::string>());
            } else {
              logger::get_logger().error("complex_modifications json error: input_sources.input_mode_id should be string: {0}", json.dump());
            }
          } else {
            logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
          }
        }
      } else {
        logger::get_logger().error("complex_modifications json error: input_sources should be array of object: {0}", json.dump());
      }
    }

    bool is_matched(const manipulator_environment::input_source& input_source) const {
      if (language_) {
        if (regex_search(std::begin(input_source.get_language()),
                         std::end(input_source.get_language()),
                         *language_)) {
          return true;
        }
      }

      if (input_source_id_) {
        if (regex_search(std::begin(input_source.get_input_source_id()),
                         std::end(input_source.get_input_source_id()),
                         *input_source_id_)) {
          return true;
        }
      }

      if (input_mode_id_) {
        if (regex_search(std::begin(input_source.get_input_mode_id()),
                         std::end(input_source.get_input_mode_id()),
                         *input_mode_id_)) {
          return true;
        }
      }

      return false;
    }

  private:
    boost::optional<std::regex> language_;
    boost::optional<std::regex> input_source_id_;
    boost::optional<std::regex> input_mode_id_;
  };

  type type_;
  std::vector<entry> entries_;

  mutable boost::optional<std::pair<manipulator_environment::input_source, bool>> cached_result_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
