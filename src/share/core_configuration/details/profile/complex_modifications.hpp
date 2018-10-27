#pragma once

#include "json_utility.hpp"

namespace krbn {
namespace core_configuration {
namespace details {
class complex_modifications final {
public:
  class parameters final {
  public:
    parameters(void) : parameters(nlohmann::json::object()) {
    }

    parameters(const nlohmann::json& json) : json_(json),
                                             basic_simultaneous_threshold_milliseconds_(50),
                                             basic_to_if_alone_timeout_milliseconds_(1000),
                                             basic_to_if_held_down_threshold_milliseconds_(500),
                                             basic_to_delayed_action_delay_milliseconds_(500) {
      update(json);
    }

    nlohmann::json to_json(void) const {
      auto j = json_;
      for (const auto& pair : make_map()) {
        j[pair.first] = pair.second;
      }
      return j;
    }

    int get_basic_simultaneous_threshold_milliseconds(void) const {
      return basic_simultaneous_threshold_milliseconds_;
    }

    int get_basic_to_if_alone_timeout_milliseconds(void) const {
      return basic_to_if_alone_timeout_milliseconds_;
    }

    int get_basic_to_if_held_down_threshold_milliseconds(void) const {
      return basic_to_if_held_down_threshold_milliseconds_;
    }

    int get_basic_to_delayed_action_delay_milliseconds(void) const {
      return basic_to_delayed_action_delay_milliseconds_;
    }

    void update(const nlohmann::json& json) {
      for (const auto& pair : make_map()) {
        if (auto v = json_utility::find_optional<int>(json, pair.first)) {
          const_cast<int&>(pair.second) = *v;
        }
      }

      normalize();
    }

    boost::optional<int> get_value(const std::string& name) const {
      auto map = make_map();
      auto it = map.find(name);
      if (it != std::end(map)) {
        return it->second;
      }
      return boost::none;
    }

    void set_value(const std::string& name, int value) {
      auto map = make_map();
      auto it = map.find(name);
      if (it != std::end(map)) {
        const_cast<int&>(it->second) = value;
      }

      normalize();
    }

  private:
    void normalize(void) {
      normalize(basic_simultaneous_threshold_milliseconds_, 0, 1000, "basic.simultaneous_threshold_milliseconds");
      normalize(basic_to_if_alone_timeout_milliseconds_, 0, boost::none, "basic.to_if_alone_timeout_milliseconds");
      normalize(basic_to_if_held_down_threshold_milliseconds_, 0, boost::none, "basic.to_if_held_down_threshold_milliseconds");
      normalize(basic_to_delayed_action_delay_milliseconds_, 0, boost::none, "basic.to_delayed_action_delay_milliseconds");
    }

    void normalize(int& value, boost::optional<int> min, boost::optional<int> max, const std::string& name) {
      if (min) {
        if (value < *min) {
          logger::get_logger().warn("{0} should be >= {1}.", name, *min);
        }
        value = std::max(value, *min);
      }

      if (max) {
        if (value > *max) {
          logger::get_logger().warn("{0} should be <= {1}.", name, *max);
        }
        value = std::min(value, *max);
      }
    }

    std::unordered_map<std::string, const int&> make_map(void) const {
      return {
          {"basic.simultaneous_threshold_milliseconds", basic_simultaneous_threshold_milliseconds_},
          {"basic.to_if_alone_timeout_milliseconds", basic_to_if_alone_timeout_milliseconds_},
          {"basic.to_if_held_down_threshold_milliseconds", basic_to_if_held_down_threshold_milliseconds_},
          {"basic.to_delayed_action_delay_milliseconds", basic_to_delayed_action_delay_milliseconds_},
      };
    }

    nlohmann::json json_;
    int basic_simultaneous_threshold_milliseconds_;
    int basic_to_if_alone_timeout_milliseconds_;
    int basic_to_if_held_down_threshold_milliseconds_;
    int basic_to_delayed_action_delay_milliseconds_;
  };

  class rule final {
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

      manipulator(const nlohmann::json& json, const parameters& parameters) : json_(json),
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

      const parameters& get_parameters(void) const {
        return parameters_;
      }

    private:
      nlohmann::json json_;
      std::vector<condition> conditions_;
      parameters parameters_;
    };

    rule(const nlohmann::json& json, const parameters& parameters) : json_(json) {
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
    boost::optional<std::string> find_description(const nlohmann::json& json) const {
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

      return boost::none;
    }

    nlohmann::json json_;
    std::vector<manipulator> manipulators_;
    std::string description_;
  };

  complex_modifications(const nlohmann::json& json) : json_(json),
                                                      parameters_(json_utility::find_copy(json, "parameters", nlohmann::json())) {
    if (auto v = json_utility::find_array(json, "rules")) {
      for (const auto& j : *v) {
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

  const parameters& get_parameters(void) const {
    return parameters_;
  }

  void set_parameter_value(const std::string& name, int value) {
    parameters_.set_value(name, value);
  }

  const std::vector<rule>& get_rules(void) const {
    return rules_;
  }

  void push_back_rule(const rule& rule) {
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

  boost::optional<std::pair<int, int>> minmax_parameter_value(const std::string& name) const {
    boost::optional<std::pair<int, int>> result;

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
  parameters parameters_;
  std::vector<rule> rules_;
};

inline void to_json(nlohmann::json& json, const complex_modifications& complex_modifications) {
  json = complex_modifications.to_json();
}

inline void to_json(nlohmann::json& json, const complex_modifications::rule& rule) {
  json = rule.get_json();
}

inline void to_json(nlohmann::json& json, const complex_modifications::parameters& parameters) {
  json = parameters.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
