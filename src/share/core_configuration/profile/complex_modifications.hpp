#pragma once

class complex_modifications final {
public:
  class parameters final {
  public:
    parameters(void) : json_(nlohmann::json::object()) {
    }

    parameters(const nlohmann::json& json) : json_(json),
                                             basic_to_if_alone_timeout_milliseconds_(1000) {
      update(json);
    }

    void update(const nlohmann::json& json) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        if (it.value().is_number()) {
          set_value(it.key(), it.value());
        }
      }
    }

    void set_value(const std::string& name, int value) {
      if (name == "basic.to_if_alone_timeout_milliseconds") {
        basic_to_if_alone_timeout_milliseconds_ = value;
      }
    }

    int get_basic_to_if_alone_timeout_milliseconds(void) const {
      return basic_to_if_alone_timeout_milliseconds_;
    }

    void set_basic_to_if_alone_timeout_milliseconds(int value) {
      basic_to_if_alone_timeout_milliseconds_ = value;
    }

  private:
    nlohmann::json json_;
    int basic_to_if_alone_timeout_milliseconds_;
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
        {
          const std::string key = "conditions";
          if (json.find(key) != json.end() && json[key].is_array()) {
            for (const auto& j : json[key]) {
              conditions_.emplace_back(j);
            }
          }
        }
        {
          const std::string key = "parameters";
          if (json.find(key) != json.end()) {
            parameters_.update(json[key]);
          }
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
      {
        const std::string key = "manipulators";
        if (json.find(key) != json.end() && json[key].is_array()) {
          for (const auto& j : json[key]) {
            manipulators_.emplace_back(j, parameters);
          }
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
      {
        const std::string key = "description";
        if (json.find(key) != json.end()) {
          if (json[key].is_string()) {
            return json[key].get<std::string>();
          } else {
            return std::string("");
          }
        }
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
                                                      parameters_(json.find("parameters") != std::end(json) ? json["parameters"] : nlohmann::json()) {
    {
      const std::string key = "rules";
      if (json_.find(key) != json_.end() && json_[key].is_array()) {
        for (const auto& j : json_[key]) {
          rules_.emplace_back(j, parameters_);
        }
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["rules"] = rules_;
    return j;
  }

  const parameters& get_parameters(void) const {
    return parameters_;
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

private:
  nlohmann::json json_;
  parameters parameters_;
  std::vector<rule> rules_;
};
