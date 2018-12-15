#pragma once

#include "complex_modifications_parameters.hpp"
#include "json_utility.hpp"

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
      if (auto v = json_utility::find_array(json, "conditions")) {
        for (const auto& j : *v) {
          conditions_.emplace_back(j);
        }
      }

      if (auto v = json_utility::find_object(json, "parameters")) {
        parameters_.update(*v);
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

  private:
    nlohmann::json json_;
    std::vector<condition> conditions_;
    complex_modifications_parameters parameters_;
  };

  complex_modifications_rule(const nlohmann::json& json,
                             const complex_modifications_parameters& parameters) : json_(json) {
    if (auto v = json_utility::find_array(json, "manipulators")) {
      for (const auto& j : *v) {
        manipulators_.emplace_back(j, parameters);
      }
    }

    if (auto d = find_description(json)) {
      description_ = *d;
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
  std::optional<std::string> find_description(const nlohmann::json& json) const {
    if (auto v = json_utility::find_optional<std::string>(json, "description")) {
      return *v;
    }

    if (json.is_array() || json.is_object()) {
      for (const auto& j : json) {
        auto s = find_description(j);
        if (s) {
          return s;
        }
      }
    }

    return std::nullopt;
  }

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
