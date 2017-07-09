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
                                                      type_(type::frontmost_application_if),
                                                      cached_update_count_(0),
                                                      cached_result_(true) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "type") {
          if (json.find(key) != std::end(json) && json[key].is_string()) {
            if (json[key] == "frontmost_application_if") {
              type_ = type::frontmost_application_if;
            }
            if (json[key] == "frontmost_application_unless") {
              type_ = type::frontmost_application_unless;
            }
          }
        } else if (key == "bundle_identifiers") {
          if (json.find(key) != std::end(json) && json[key].is_array()) {
            for (const auto& j : json[key]) {
              if (j.is_string()) {
                std::string s = j;
                try {
                  std::regex r(s);
                  bundle_identifiers_.push_back(r);
                } catch (std::exception& e) {
                  logger::get_logger().error("regex error: \"{0}\" {1}", s, e.what());
                }
              }
            }
          }
        } else if (key == "file_paths") {
          if (json.find(key) != std::end(json) && json[key].is_array()) {
            for (const auto& j : json[key]) {
              if (j.is_string()) {
                std::string s = j;
                try {
                  std::regex r(s);
                  file_paths_.push_back(r);
                } catch (std::exception& e) {
                  logger::get_logger().error("regex error: \"{0}\" {1}", s, e.what());
                }
              }
            }
          }
        } else {
          logger::get_logger().error("unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  virtual ~frontmost_application(void) {
  }

  virtual bool is_fulfilled(const manipulator_environment& manipulator_environment) const {
    if (manipulator_environment.get_frontmost_application().get_update_count() == cached_update_count_) {
      return cached_result_;
    }

    auto& current_bundle_identifier = manipulator_environment.get_frontmost_application().get_bundle_identifier();
    auto& current_file_path = manipulator_environment.get_frontmost_application().get_file_path();

    // Bundle identifiers

    for (const auto& b : bundle_identifiers_) {
      if (regex_search(std::begin(current_bundle_identifier),
                       std::end(current_bundle_identifier),
                       b)) {
        switch (type_) {
          case type::frontmost_application_if:
            return true;
          case type::frontmost_application_unless:
            return false;
        }
      }
    }

    // File paths

    for (const auto& f : file_paths_) {
      if (regex_search(std::begin(current_file_path),
                       std::end(current_file_path),
                       f)) {
        switch (type_) {
          case type::frontmost_application_if:
            return true;
          case type::frontmost_application_unless:
            return false;
        }
      }
    }

    // Not found

    switch (type_) {
      case type::frontmost_application_if:
        return false;
      case type::frontmost_application_unless:
        return true;
    }
  }

private:
  type type_;
  std::vector<std::regex> bundle_identifiers_;
  std::vector<std::regex> file_paths_;

  uint32_t cached_update_count_;
  bool cached_result_;
};
} // namespace conditions
} // namespace details
} // namespace manipulator
} // namespace krbn
