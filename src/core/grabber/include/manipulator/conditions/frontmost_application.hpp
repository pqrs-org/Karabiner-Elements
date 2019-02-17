#pragma once

#include "base.hpp"
#include <regex>
#include <string>
#include <vector>

namespace krbn {
namespace manipulator {
namespace conditions {
class frontmost_application final : public base {
public:
  enum class type {
    frontmost_application_if,
    frontmost_application_unless,
  };

  frontmost_application(const nlohmann::json& json) : base(),
                                                      type_(type::frontmost_application_if) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("frontmost_application must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      // key is always std::string.

      if (key == "type") {
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("{0} must be string, but is `{1}`", key, value.dump()));
        }

        auto t = value.get<std::string>();

        if (t == "frontmost_application_if") {
          type_ = type::frontmost_application_if;
        } else if (t == "frontmost_application_unless") {
          type_ = type::frontmost_application_unless;
        } else {
          throw pqrs::json::unmarshal_error(fmt::format("unknown type `{0}`", t));
        }

      } else if (key == "bundle_identifiers") {
        if (!value.is_array()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
        }

        for (const auto& j : value) {
          if (!j.is_string()) {
            throw pqrs::json::unmarshal_error(fmt::format("bundle_identifiers entry must be string, but is `{0}`", j.dump()));
          }

          auto s = j.get<std::string>();

          try {
            std::regex r(s);
            bundle_identifiers_.push_back(r);
          } catch (std::exception& e) {
            throw pqrs::json::unmarshal_error(fmt::format("{0}: `{1}:{2}`", e.what(), key, value.dump()));
          }
        }

      } else if (key == "file_paths") {
        if (!value.is_array()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
        }

        for (const auto& j : value) {
          if (!j.is_string()) {
            throw pqrs::json::unmarshal_error(fmt::format("file_paths entry must be string, but is `{0}`", j.dump()));
          }

          if (j.is_string()) {
            std::string s = j;
            try {
              std::regex r(s);
              file_paths_.push_back(r);
            } catch (std::exception& e) {
              throw pqrs::json::unmarshal_error(fmt::format("{0}: `{1}:{2}`", e.what(), key, value.dump()));
            }
          }
        }

      } else if (key == "description") {
        // Do nothing

      } else {
        throw pqrs::json::unmarshal_error(fmt::format("unknown key `{0}` in `{1}`", key, json.dump()));
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
} // namespace manipulator
} // namespace krbn
