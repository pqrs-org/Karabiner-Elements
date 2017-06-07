#pragma once

class complex_modifications final {
public:
  class parameters final {
  public:
    parameters(const nlohmann::json& json) : json_(json) {
    }

    nlohmann::json to_json(void) const {
      return json_;
    };

  private:
    nlohmann::json json_;
  };

  class rule final {
  public:
    class condition {
    public:
      condition(const nlohmann::json& json) : json_(json) {
      }

      nlohmann::json to_json(void) const {
        return json_;
      }

      const nlohmann::json& get_json(void) const {
        return json_;
      }

    private:
      nlohmann::json json_;
    };

    class manipulator {
    public:
      manipulator(const nlohmann::json& json) : json_(json) {
      }

      nlohmann::json to_json(void) const {
        return json_;
      }

      const nlohmann::json& get_json(void) const {
        return json_;
      }

    private:
      nlohmann::json json_;
    };

    rule(const nlohmann::json& json) : json_(json) {
      {
        const std::string key = "conditions";
        if (json.find(key) != json.end() && json[key].is_array()) {
          for (const auto& j : json[key]) {
            conditions_.emplace_back(j);
          }
        }
      }
      {
        const std::string key = "manipulators";
        if (json.find(key) != json.end() && json[key].is_array()) {
          for (const auto& j : json[key]) {
            manipulators_.emplace_back(j);
          }
        }
      }
    }

    nlohmann::json to_json(void) const {
      nlohmann::json j = json_;
      j["conditions"] = conditions_;
      j["manipulators"] = manipulators_;
      return j;
    }

    const std::vector<condition>& get_conditions(void) const {
      return conditions_;
    }

    const std::vector<manipulator>& get_manipulators(void) const {
      return manipulators_;
    }

  private:
    nlohmann::json json_;
    std::vector<condition> conditions_;
    std::vector<manipulator> manipulators_;
  };

  complex_modifications(const nlohmann::json& json) : json_(json),
                                                      parameters_(json.find("parameters") != std::end(json) ? json["parameters"] : nlohmann::json()) {
    {
      const std::string key = "rules";
      if (json_.find(key) != json_.end() && json_[key].is_array()) {
        for (const auto& j : json_[key]) {
          rules_.emplace_back(j);
        }
      }
    }
  }

  nlohmann::json to_json(void) const {
    return json_;
  };

  const parameters& get_parameters(void) const {
    return parameters_;
  }

  const std::vector<rule>& get_rules(void) const {
    return rules_;
  }

private:
  nlohmann::json json_;
  parameters parameters_;
  std::vector<rule> rules_;
};
