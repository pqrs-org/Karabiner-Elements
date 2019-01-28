#pragma once

#include "manipulator/details/conditions/base.hpp"
#include <regex>
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace details {
namespace conditions {
class frontmost_application final : public base {
public:
  enum class type {
    frontmost_application_if,
    frontmost_application_unless,
  };

  frontmost_application(const nlohmann::json& json) : base(),
                                                      type_(type::frontmost_application_if) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "type") {
          if (value.is_string()) {
            if (value == "frontmost_application_if") {
              type_ = type::frontmost_application_if;
            }
            if (value == "frontmost_application_unless") {
              type_ = type::frontmost_application_unless;
            }
          }
        } else if (key == "bundle_identifiers") {
          if (value.is_array()) {
            for (const auto& j : value) {
              if (j.is_string()) {
                std::string s = j;
                try {
                  std::regex r(s);
                  bundle_identifiers_.push_back(r);
                } catch (std::exception& e) {
                  logger::get_logger()->error("complex_modifications json error: Regex error: \"{0}\" {1}", s, e.what());
                }
              }
            }
          }
        } else if (key == "file_paths") {
          if (value.is_array()) {
            for (const auto& j : value) {
              if (j.is_string()) {
                std::string s = j;
                try {
                  std::regex r(s);
                  file_paths_.push_back(r);
                } catch (std::exception& e) {
                  logger::get_logger()->error("complex_modifications json error: Regex error: \"{0}\" {1}", s, e.what());
                }
              }
            }
          }
        } else {
          logger::get_logger()->error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  virtual ~frontmost_application(void) {
  }

  virtual bool is_fulfilled(const event_queue::entry& entry,
                            const manipulator_environment& manipulator_environment) const {
    if (cached_result_ && cached_result_->first == manipulator_environment.get_frontmost_application()) {
      return cached_result_->second;
    }

    bool result = false;

    // Bundle identifiers

    if (auto& current_bundle_identifier = manipulator_environment.get_frontmost_application().get_bundle_identifier()) {
      for (const auto& b : bundle_identifiers_) {
        if (regex_search(std::begin(*current_bundle_identifier),
                         std::end(*current_bundle_identifier),
                         b)) {
          switch (type_) {
            case type::frontmost_application_if:
              result = true;
              goto finish;
            case type::frontmost_application_unless:
              result = false;
              goto finish;
          }
        }
      }
    }

    // File paths

    if (auto& current_file_path = manipulator_environment.get_frontmost_application().get_file_path()) {
      for (const auto& f : file_paths_) {
        if (regex_search(std::begin(*current_file_path),
                         std::end(*current_file_path),
                         f)) {
          switch (type_) {
            case type::frontmost_application_if:
              result = true;
              goto finish;
            case type::frontmost_application_unless:
              result = false;
              goto finish;
          }
        }
      }
    }

    // Not found

    switch (type_) {
      case type::frontmost_application_if:
        result = false;
        goto finish;
      case type::frontmost_application_unless:
        result = true;
        goto finish;
    }

  finish:
    cached_result_ = std::make_pair(manipulator_environment.get_frontmost_application(), result);
    return result;
  }

private:
  type type_;
  std::vector<std::regex> bundle_identifiers_;
  std::vector<std::regex> file_paths_;

  mutable std::optional<std::pair<pqrs::osx::frontmost_application_monitor::application, bool>> cached_result_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
