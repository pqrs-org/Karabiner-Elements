#pragma once

#include "complex_modifications_parameters.hpp"
#include <pqrs/json.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
class complex_modifications_rule final {
public:
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

    manipulator(const nlohmann::json& json,
                const complex_modifications_parameters& parameters) : json_(json),
                                                                      parameters_(parameters) {
      if (!json.is_object()) {
        throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
      }

      for (const auto& [key, value] : json.items()) {
        if (key == "conditions") {
          if (!value.is_array()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
          }

          for (const auto& j : value) {
            conditions_.emplace_back(j);
          }

        } else if (key == "parameters") {
          if (!value.is_object()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be object, but is `{1}`", key, value.dump()));
          }

          parameters_.update(value);

        } else if (key == "description") {
          if (!value.is_string()) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be string, but is `{1}`", key, value.dump()));
          }

          description_ = value.get<std::string>();

        } else {
          // Allow unknown key
        }
      }
    }

    const nlohmann::json& get_json(void) const {
      return json_;
    }

    const std::vector<condition>& get_conditions(void) const {
      return conditions_;
    }

    const complex_modifications_parameters& get_parameters(void) const {
      return parameters_;
    }

    const std::string& get_description(void) const {
      return description_;
    }

  private:
    nlohmann::json json_;
    std::vector<condition> conditions_;
    complex_modifications_parameters parameters_;
    std::string description_;
  };

  complex_modifications_rule(const nlohmann::json& json,
                             const complex_modifications_parameters& parameters) : json_(json) {
    if (!json.is_object()) {
      throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
    }

    for (const auto& [key, value] : json.items()) {
      if (key == "manipulators") {
        if (!value.is_array()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be array, but is `{1}`", key, value.dump()));
        }

        for (const auto& j : value) {
          try {
            manipulators_.emplace_back(j, parameters);
          } catch (const pqrs::json::unmarshal_error& e) {
            throw pqrs::json::unmarshal_error(fmt::format("`{0}` entry error: {1}", key, e.what()));
          }
        }

      } else if (key == "description") {
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be string, but is `{1}`", key, value.dump()));
        }

        description_ = value.get<std::string>();

      } else if (key == "available_since") {
        // (TODO) broken link, can not find this key in any documentation
        // `available_since` is used in <https://pqrs.org/osx/karabiner/complex_modifications/>.
        if (!value.is_string()) {
          throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be string, but is `{1}`", key, value.dump()));
        }

      } else {
        // Allow unknown key
      }
    }

    // Use manipulators_'s description if needed.
    if (description_.empty()) {
      for (const auto& m : manipulators_) {
        if (!m.get_description().empty()) {
          description_ = m.get_description();
          break;
        }
      }
    }
  }

  const nlohmann::json& get_json(void) const {
    return json_;
  }

  const std::vector<manipulator>& get_manipulators(void) const {
    return manipulators_;
  }

  const std::string& get_description(void) const {
    return description_;
  }

private:
  nlohmann::json json_;
  std::vector<manipulator> manipulators_;
  std::string description_;
};

inline void to_json(nlohmann::json& json, const complex_modifications_rule& rule) {
  json = rule.get_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
