#pragma once

#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "manipulator/manipulator_factory.hpp"
#include <pqrs/string.hpp>

namespace krbn {
class complex_modifications_assets_file final {
public:
  complex_modifications_assets_file(const std::string& file_path) : file_path_(file_path) {
    std::ifstream stream(file_path);
    if (!stream) {
      throw std::runtime_error(std::string("failed to open ") + file_path);
    } else {
      auto json = nlohmann::json::parse(stream);

      if (!json.is_object()) {
        throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
      }

      for (const auto& [key, value] : json.items()) {
        if (key == "title") {
          if (!value.is_string()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be string, but is `{1}`", key, value.dump()));
          }

          title_ = value.get<std::string>();

        } else if (key == "maintainers") {
          // `maintainers` is used in <https://pqrs.org/osx/karabiner/complex_modifications/>.
          if (!value.is_array()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
          }

        } else if (key == "rules") {
          if (!value.is_array()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
          }

          core_configuration::details::complex_modifications_parameters parameters;
          for (const auto& j : value) {
            try {
              rules_.emplace_back(j, parameters);
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
            }
          }

        } else {
          // Ignore unknown keys
        }
      }
    }
  }

  const std::string& get_file_path(void) const {
    return file_path_;
  }

  const std::string& get_title(void) const {
    return title_;
  }

  const std::vector<core_configuration::details::complex_modifications_rule>& get_rules(void) const {
    return rules_;
  }

  void push_back_rule_to_core_configuration_profile(core_configuration::details::profile& profile,
                                                    size_t index) {
    if (index < rules_.size()) {
      profile.push_back_complex_modifications_rule(rules_[index]);
    }
  }

  void unlink_file(void) const {
    unlink(file_path_.c_str());
  }

  bool user_file(void) const {
    return pqrs::string::starts_with(file_path_, constants::get_user_complex_modifications_assets_directory());
  }

  std::vector<std::string> lint(void) const {
    std::vector<std::string> error_messages;

    for (const auto& rule : rules_) {
      for (const auto& manipulator : rule.get_manipulators()) {
        try {
          manipulator::manipulator_factory::make_manipulator(manipulator.get_json(),
                                                             manipulator.get_parameters());
          for (const auto& c : manipulator.get_conditions()) {
            manipulator::manipulator_factory::make_condition(c.get_json());
          }

        } catch (const std::exception& e) {
          error_messages.push_back(fmt::format("`{0}` error: {1}",
                                               rule.get_description(),
                                               e.what()));
        }
      }
    }

    return error_messages;
  }

private:
  std::string file_path_;
  std::string title_;
  std::vector<core_configuration::details::complex_modifications_rule> rules_;
};
} // namespace krbn
