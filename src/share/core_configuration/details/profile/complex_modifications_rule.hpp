#pragma once

#include "complex_modifications_parameters.hpp"
#include "duktape_utility.hpp"
#include "json_utility.hpp"
#include <pqrs/json.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
class complex_modifications_rule final {
public:
  enum class code_type {
    json,
    javascript,
  };

  class manipulator {
  public:
    class condition {
    public:
      condition(const nlohmann::json& json) : json_(json) {
      }

      const nlohmann::json& get_json(void) const {
        return json_;
      }

    private:
      nlohmann::json json_;
    };

    manipulator(const manipulator&) = delete;

    manipulator(const nlohmann::json& json,
                pqrs::not_null_shared_ptr_t<const core_configuration::details::complex_modifications_parameters> parameters,
                error_handling error_handling)
        : json_(json),
          parameters_(std::make_shared<complex_modifications_parameters>(parameters->to_json(), error_handling)) {
      pqrs::json::requires_object(json, "json");

      for (const auto& [key, value] : json.items()) {
        if (key == "conditions") {
          pqrs::json::requires_array(value, "`" + key + "`");

          for (const auto& j : value) {
            conditions_.emplace_back(j);
          }

        } else if (key == "parameters") {
          parameters_->update(value,
                              error_handling);

        } else if (key == "description") {
          pqrs::json::requires_string(value, "`" + key + "`");

          description_ = value.get<std::string>();

        } else {
          // Allow unknown key
        }
      }
    }

    nlohmann::json to_json(void) const {
      return json_;
    }

    const std::vector<condition>& get_conditions(void) const {
      return conditions_;
    }

    pqrs::not_null_shared_ptr_t<complex_modifications_parameters> get_parameters(void) const {
      return parameters_;
    }

    const std::string& get_description(void) const {
      return description_;
    }

  private:
    nlohmann::json json_;
    std::vector<condition> conditions_;
    pqrs::not_null_shared_ptr_t<complex_modifications_parameters> parameters_;
    std::string description_;
  };

  complex_modifications_rule(const complex_modifications_rule&) = delete;

  complex_modifications_rule(const nlohmann::json& json,
                             pqrs::not_null_shared_ptr_t<const core_configuration::details::complex_modifications_parameters> parameters,
                             error_handling error_handling)
      : json_(json),
        code_type_(code_type::json) {
    auto resolved_json = resolve_code(json, error_handling);

    helper_values_.push_back_value<bool>("enabled",
                                         enabled_,
                                         true);

    pqrs::json::requires_object(resolved_json, "json");

    helper_values_.update_value(resolved_json, error_handling);

    for (const auto& [key, value] : resolved_json.items()) {
      if (key == "manipulators") {
        pqrs::json::requires_array(value, "`" + key + "`");

        for (const auto& j : value) {
          try {
            auto m = std::make_shared<manipulator>(j,
                                                   parameters,
                                                   error_handling);
            manipulators_.push_back(m);
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
          }
        }

      } else if (key == "description") {
        pqrs::json::requires_string(value, "`" + key + "`");

        description_ = value.get<std::string>();

      } else if (key == "available_since") {
        // `available_since` is used in <https://ke-complex-modifications.pqrs.org/>.
        pqrs::json::requires_string(value, "`" + key + "`");

      } else {
        // Allow unknown key
      }
    }

    if (manipulators_.empty()) {
      throw pqrs::json::unmarshal_error(fmt::format("`manipulators` is missing or empty in {0}", pqrs::json::dump_for_error_message(resolved_json)));
    }

    // Use manipulators_'s description if needed.
    if (description_.empty()) {
      for (const auto& m : manipulators_) {
        if (!m->get_description().empty()) {
          description_ = m->get_description();
          break;
        }
      }
    }
  }

  complex_modifications_rule(const std::string& code_string,
                             code_type type,
                             pqrs::not_null_shared_ptr_t<const core_configuration::details::complex_modifications_parameters> parameters,
                             error_handling error_handling)
      : complex_modifications_rule(
            type == code_type::json
                ? krbn::json_utility::parse_jsonc(code_string)
                : nlohmann::json::object({
                      {"eval_js", code_string},
                  }),
            parameters,
            error_handling) {
  }

  nlohmann::json to_json(void) const {
    auto j = json_;

    helper_values_.update_json(j);

    return j;
  }

  const std::vector<pqrs::not_null_shared_ptr_t<manipulator>>& get_manipulators(void) const {
    return manipulators_;
  }

  const bool& get_enabled(void) const {
    return enabled_;
  }

  void set_enabled(bool value) {
    enabled_ = value;
  }

  const std::string& get_description(void) const {
    return description_;
  }

  const code_type get_code_type(void) const {
    return code_type_;
  }

  const std::string get_code_string(void) const {
    return code_string_;
  }

private:
  nlohmann::json resolve_code(const nlohmann::json& json,
                              error_handling error_handling) {
    if (json.is_object() && json.contains("eval_js")) {
      const auto& value = json.at("eval_js");
      pqrs::json::requires_string(value, "`eval_js`");

      try {
        code_type_ = code_type::javascript;
        code_string_ = value.get<std::string>();

        auto result = krbn::duktape_utility::eval_string_to_json(code_string_);

        if (json.contains("enabled")) {
          result.json["enabled"] = json["enabled"];
        }

        return result.json;
      } catch (const std::exception& e) {
        throw pqrs::json::unmarshal_error(fmt::format("`eval_js` error: {0}", e.what()));
      }
    }

    code_string_ = krbn::json_utility::dump(json);
    return json;
  }

  nlohmann::json json_;
  std::vector<pqrs::not_null_shared_ptr_t<manipulator>> manipulators_;
  bool enabled_;
  std::string description_;
  code_type code_type_;
  std::string code_string_;
  configuration_json_helper::helper_values helper_values_;
};
} // namespace details
} // namespace core_configuration
} // namespace krbn
