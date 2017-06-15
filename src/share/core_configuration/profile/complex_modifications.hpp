#pragma once

class complex_modifications final {
public:
  class parameters final {
  public:
    class basic final {
    public:
      basic(void) : to_if_alone_timeout_milliseconds_(1000) {
      }

      void update(const nlohmann::json& json) {
        {
          const std::string key = "to_if_alone_timeout_milliseconds";
          if (json.find(key) != json.end() && json[key].is_number()) {
            to_if_alone_timeout_milliseconds_ = json[key];
          }
        }
      }

      int get_to_if_alone_timeout_milliseconds(void) const {
        return to_if_alone_timeout_milliseconds_;
      }

    private:
      int to_if_alone_timeout_milliseconds_;
    };

    parameters(void) : json_(nlohmann::json::object()) {
    }

    parameters(const nlohmann::json& json) : json_(json) {
      update(json);
    }

    void update(const nlohmann::json& json) {
      {
        const std::string key = "basic";
        if (json.find(key) != json.end()) {
          basic_.update(json[key]);
        }
      }
    }

    const basic& get_basic(void) const {
      return basic_;
    }

  private:
    nlohmann::json json_;
    basic basic_;
  };

  class rule final {
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

    class manipulator {
    public:
      manipulator(const nlohmann::json& json, const parameters& parameters) : json_(json),
                                                                              parameters_(parameters) {
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

      const parameters& get_parameters(void) const {
        return parameters_;
      }

    private:
      nlohmann::json json_;
      parameters parameters_;
    };

    rule(const nlohmann::json& json, const parameters& parameters) : json_(json) {
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
            manipulators_.emplace_back(j, parameters);
          }
        }
      }

      if (auto d = find_description(json)) {
        description_ = *d;
      }
    }

    const std::vector<condition>& get_conditions(void) const {
      return conditions_;
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
    std::vector<condition> conditions_;
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
    return json_;
  }

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
