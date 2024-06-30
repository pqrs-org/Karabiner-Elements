#pragma once

#include "complex_modifications_parameters.hpp"
#include "complex_modifications_rule.hpp"
#include "vector_utility.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class complex_modifications final {
public:
  complex_modifications(const complex_modifications&) = delete;

  complex_modifications(void)
      : complex_modifications(nlohmann::json::object(),
                              krbn::core_configuration::error_handling::loose) {
  }

  complex_modifications(const nlohmann::json& json,
                        error_handling error_handling)
      : json_(json),
        parameters_(std::make_shared<complex_modifications_parameters>(nlohmann::json::object(),
                                                                       error_handling)) {
    helper_values_.push_back_object<complex_modifications_parameters>("parameters",
                                                                      parameters_);

    pqrs::json::requires_object(json, "json");

    helper_values_.update_value(json, error_handling);

    // Load rules_

    if (auto v = pqrs::json::find_array(json, "rules")) {
      for (const auto& j : v->value()) {
        rules_.push_back(std::make_shared<complex_modifications_rule>(j,
                                                                      parameters_,
                                                                      error_handling));
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;

    helper_values_.update_json(j);

    {
      auto key = "rules";

      auto rules_json = nlohmann::json::array();
      for (const auto& r : rules_) {
        auto jj = r->to_json();
        if (!jj.empty()) {
          rules_json.push_back(jj);
        }
      }

      if (rules_json.empty()) {
        j.erase(key);
      } else {
        j[key] = rules_json;
      }
    }

    return j;
  }

  gsl::not_null<std::shared_ptr<complex_modifications_parameters>> get_parameters(void) const {
    return parameters_;
  }

  const std::vector<gsl::not_null<std::shared_ptr<complex_modifications_rule>>>& get_rules(void) const {
    return rules_;
  }

  void push_front_rule(gsl::not_null<std::shared_ptr<complex_modifications_rule>> rule) {
    rules_.insert(std::begin(rules_), rule);
  }

  void replace_rule(size_t index, gsl::not_null<std::shared_ptr<complex_modifications_rule>> rule) {
    if (index < rules_.size()) {
      rules_[index] = rule;
    }
  }

  void erase_rule(size_t index) {
    if (index < rules_.size()) {
      rules_.erase(std::begin(rules_) + index);
    }
  }

  void move_rule(size_t source_index, size_t destination_index) {
    vector_utility::move_element(rules_, source_index, destination_index);
  }

private:
  nlohmann::json json_;
  gsl::not_null<std::shared_ptr<complex_modifications_parameters>> parameters_;
  std::vector<gsl::not_null<std::shared_ptr<complex_modifications_rule>>> rules_;
  configuration_json_helper::helper_values helper_values_;
};
} // namespace details
} // namespace core_configuration
} // namespace krbn
