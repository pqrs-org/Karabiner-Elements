#pragma once

class complex_modification final {
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

    protected:
      nlohmann::json json_;
    };

    class manipulator {
    public:
      manipulator(const nlohmann::json& json) : json_(json) {
      }

      nlohmann::json to_json(void) const {
        return json_;
      }

    protected:
      nlohmann::json json_;
    };

  private:
    std::vector<condition> conditions_;
    std::vector<manipulator> manipulators_;
  };

  complex_modification(const nlohmann::json& json) : json_(json),
                                                     parameters_(json.find("parameters") != std::end(json) ? json["parameters"] : nlohmann::json()) {
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    return j;
  };

private:
  nlohmann::json json_;
  parameters parameters_;
  std::vector<rule> rules_;
};
