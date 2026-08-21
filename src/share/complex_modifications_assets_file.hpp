#pragma once

#include "complex_modifications_utility.hpp"
#include "constants.hpp"
#include "core_configuration/core_configuration.hpp"
#include "filesystem_utility.hpp"
#include "json_utility.hpp"
#include "manipulator/manipulator_factory.hpp"

namespace krbn {
// Loads the following complex modifications file formats:
// - An asset JSON/JSONC file containing a title and multiple rules.
// - A JSON/JSONC file containing a single rule accepted by the complex modifications editor.
// - A JavaScript file containing a single rule accepted by the complex modifications editor.
//
// A single-rule file is exposed as an asset containing one rule, with the rule's resolved
// description used as the asset title.
class complex_modifications_assets_file final {
public:
  complex_modifications_assets_file(const complex_modifications_assets_file&) = delete;

  complex_modifications_assets_file(const std::filesystem::path& file_path,
                                    core_configuration::error_handling error_handling)
      : file_path_(file_path) {
    std::ifstream stream(file_path);
    if (!stream) {
      throw std::runtime_error(fmt::format("failed to open {0}", file_path.string()));
    }

    auto code = std::string(std::istreambuf_iterator<char>(stream),
                            std::istreambuf_iterator<char>());
    load(code, error_handling);
  }

  complex_modifications_assets_file(const std::filesystem::path& file_path,
                                    const std::string& code,
                                    core_configuration::error_handling error_handling)
      : file_path_(file_path) {
    load(code, error_handling);
  }

  [[nodiscard]] const std::filesystem::path& get_file_path() const {
    return file_path_;
  }

  [[nodiscard]] const std::string& get_title() const {
    return title_;
  }

  [[nodiscard]] const std::vector<pqrs::not_null_shared_ptr_t<core_configuration::details::complex_modifications_rule>>& get_rules() const {
    return rules_;
  }

  void push_front_rule_to_core_configuration_profile(core_configuration::details::profile& profile,
                                                     size_t index) {
    if (index < rules_.size()) {
      auto c = profile.get_complex_modifications();
      c->push_front_rule(rules_[index]);
    }
  }

  [[nodiscard]] std::optional<std::filesystem::file_time_type> last_write_time() const {
    return filesystem_utility::last_write_time(file_path_);
  }

  void unlink_file() const {
    unlink(file_path_.c_str());
  }

  [[nodiscard]] bool user_file() const {
    return file_path_.string().starts_with(constants::get_user_complex_modifications_assets_directory().string());
  }

  std::vector<std::string> lint() const {
    std::vector<std::string> error_messages;

    for (const auto& r : rules_) {
      for (const auto& message : complex_modifications_utility::lint_rule(*r)) {
        error_messages.push_back(message);
      }
    }

    return error_messages;
  }

private:
  void load(const std::string& code,
            core_configuration::error_handling error_handling) {
    if (file_path_.extension() == ".js") {
      load_rule(code,
                core_configuration::details::complex_modifications_rule::code_type::javascript,
                error_handling);
      return;
    }

    auto json = json_utility::parse_jsonc(code);

    pqrs::json::requires_object(json, "json");

    if (json.contains("title")) {
      for (const auto& [key, value] : json.items()) {
        if (key == "title") {
          pqrs::json::requires_string(value, "`" + key + "`");

          title_ = value.get<std::string>();

        } else if (key == "maintainers") {
          // `maintainers` is used in <https://ke-complex-modifications.pqrs.org/>.
          pqrs::json::requires_array(value, "`" + key + "`");

        } else if (key == "rules") {
          pqrs::json::requires_array(value, "`" + key + "`");

          for (const auto& j : value) {
            try {
              auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
              auto r = std::make_shared<core_configuration::details::complex_modifications_rule>(j,
                                                                                                 parameters,
                                                                                                 error_handling);
              rules_.push_back(r);
            } catch (const pqrs::json::unmarshal_error& e) {
              throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
            }
          }

        } else {
          // Ignore unknown keys
        }
      }
    } else {
      load_rule(json,
                error_handling);
    }
  }

  void load_rule(const nlohmann::json& json,
                 core_configuration::error_handling error_handling) {
    auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
    auto rule_json = json;
    // Load the rule as enabled even if the distributed file specifies "enabled": false.
    rule_json.erase("enabled");
    auto rule = std::make_shared<core_configuration::details::complex_modifications_rule>(rule_json,
                                                                                          parameters,
                                                                                          error_handling);
    title_ = rule->get_description();
    rules_.push_back(rule);
  }

  void load_rule(const std::string& code,
                 core_configuration::details::complex_modifications_rule::code_type code_type,
                 core_configuration::error_handling error_handling) {
    auto parameters = std::make_shared<krbn::core_configuration::details::complex_modifications_parameters>();
    auto rule = core_configuration::details::complex_modifications_rule::make_from_code(code,
                                                                                        code_type,
                                                                                        parameters,
                                                                                        error_handling);
    title_ = rule->get_description();
    rules_.push_back(rule);
  }

  std::filesystem::path file_path_;
  std::string title_;
  std::vector<pqrs::not_null_shared_ptr_t<core_configuration::details::complex_modifications_rule>> rules_;
};
} // namespace krbn
