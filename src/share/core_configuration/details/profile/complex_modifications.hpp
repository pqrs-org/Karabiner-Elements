#pragma once

#include "complex_modifications_parameters.hpp"
#include "complex_modifications_rule.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class complex_modifications final {
public:
  complex_modifications(void) : complex_modifications(nlohmann::json::object()) {
  }

  complex_modifications(const nlohmann::json& json) : json_(json) {
    pqrs::json::requires_object(json, "json");

    // Load parameters_

    if (auto v = pqrs::json::find_json(json, "parameters")) {
      parameters_ = complex_modifications_parameters(v->value());
    }

    // Load rules_

    if (auto v = pqrs::json::find_array(json, "rules")) {
      for (const auto& j : v->value()) {
        rules_.emplace_back(j, parameters_);
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["rules"] = rules_;
    j["parameters"] = parameters_.to_json();
    return j;
  }

  const complex_modifications_parameters& get_parameters(void) const {
    return parameters_;
  }

  void set_parameter_value(const std::string& name, int value) {
    parameters_.set_value(name, value);
  }

  const std::vector<complex_modifications_rule>& get_rules(void) const {
    return rules_;
  }

  void push_back_rule(const complex_modifications_rule& rule) {
    rules_.push_back(rule);
  }

  void erase_rule(size_t index) {
    if (index < rules_.size()) {
      rules_.erase(std::begin(rules_) + index);
    }
  }

  void swap_rules(size_t index1, size_t index2) {
    if (index1 < rules_.size() &&
        index2 < rules_.size()) {
      std::swap(rules_[index1], rules_[index2]);
    }
  }

  std::optional<std::pair<int, int>> minmax_parameter_value(const std::string& name) const {
    std::optional<std::pair<int, int>> result;

    if (auto value = parameters_.get_value(name)) {
      if (!result) {
        result = std::make_pair(*value, *value);
      } else if (*value < result->first) {
        result->first = *value;
      } else if (*value > result->second) {
        result->second = *value;
      }
    }

    for (const auto& r : rules_) {
      for (const auto& m : r.get_manipulators()) {
        if (auto value = m.get_parameters().get_value(name)) {
          if (!result) {
            result = std::make_pair(*value, *value);
          } else if (*value < result->first) {
            result->first = *value;
          } else if (*value > result->second) {
            result->second = *value;
          }
        }
      }
    }

    return result;
  }

private:
  nlohmann::json json_;
  complex_modifications_parameters parameters_;
  std::vector<complex_modifications_rule> rules_;
};

inline void to_json(nlohmann::json& json, const complex_modifications& complex_modifications) {
  json = complex_modifications.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
