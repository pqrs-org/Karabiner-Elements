#pragma once

class simple_modifications final {
public:
  class definition final {
  public:
    definition(void) {
    }

    definition(const nlohmann::json& json) {
      if (json.is_object()) {
        for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
          // it.key() is always std::string.
          const auto& key = it.key();
          const auto& value = it.value();

          if (value.is_string()) {
            type_ = key;
            value_ = value;
          } else {
            logger::get_logger().error("json error: {0} should be string", value.dump());
          }
          break;
        }

      } else {
        logger::get_logger().error("json error: definition should be object {0}", json.dump());
      }
    }

    definition(const std::string& type,
               const std::string& value) : type_(type),
                                           value_(value) {
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json::object({
          {type_, value_},
      });
    }

    const std::string& get_type(void) const {
      return type_;
    }

    void set_type(const std::string& value) {
      type_ = value;
    }

    const std::string& get_value(void) const {
      return value_;
    }

    void set_value(const std::string& value) {
      value_ = value;
    }

    bool valid(void) const {
      return !type_.empty() &&
             !value_.empty();
    }

    bool compare(const definition& other) const {
      if (type_ != other.type_) {
        return SI::natural::compare<std::string>(type_, other.type_);
      } else {
        return SI::natural::compare<std::string>(value_, other.value_);
      }
    }

    bool operator==(const definition& other) const {
      // Do not compare `from_mandatory_modifiers_`.
      return type_ == other.type_ &&
             value_ == other.value_;
    }

  private:
    std::string type_;
    std::string value_;
  };

  struct definition_hash {
    std::size_t operator()(const definition& d) const {
      return std::hash<std::string>()(d.get_type()) ^
             std::hash<std::string>()(d.get_value());
    }
  };

  simple_modifications(const nlohmann::json& json) {
    handle_json(json);
  }

  nlohmann::json to_json(void) const {
    std::unordered_set<definition, definition_hash> froms;

    auto json = nlohmann::json::array();
    for (const auto& it : pairs_) {
      if (it.first.valid() &&
          it.second.valid() &&
          froms.find(it.first) == std::end(froms)) {
        json.push_back(nlohmann::json({
            {"from", it.first.to_json()},
            {"to", it.second.to_json()},
        }));
        froms.insert(it.first);
      }
    }
    return json;
  }

  const std::vector<std::pair<definition, definition>>& get_pairs(void) const {
    return pairs_;
  }

  void update(const nlohmann::json& json) {
    handle_json(json);
  }

  void push_back_pair(void) {
    pairs_.emplace_back(definition("key_code", ""),
                        definition("key_code", ""));
  }

  void erase_pair(size_t index) {
    if (index < pairs_.size()) {
      pairs_.erase(pairs_.begin() + index);
    }
  }

  void replace_pair(size_t index, const std::string& from, const std::string& to) {
    if (index < pairs_.size()) {
      pairs_[index].first.set_value(from);
      pairs_[index].second.set_value(to);
    }
  }

  void replace_second(const std::string& from, const std::string& to) {
    for (auto&& it : pairs_) {
      if (it.first.get_value() == from) {
        it.second.set_value(to);
        return;
      }
    }
  }

private:
  void handle_json(const nlohmann::json& json) {
    // Override existing definition

    if (json.is_array()) {
      // v2 format

      for (const auto& j : json) {
        if (j.is_object()) {
          std::unique_ptr<definition> from_definition;
          std::unique_ptr<definition> to_definition;

          for (auto it = j.begin(); it != j.end(); ++it) {
            // it.key() is always std::string.
            const auto& key = it.key();
            const auto& value = it.value();

            if (key == "from") {
              from_definition = std::make_unique<definition>(value);
            } else if (key == "to") {
              to_definition = std::make_unique<definition>(value);
            } else {
              logger::get_logger().error("json error: Unknown key: {0} in {1}", key, j.dump());
            }
          }

          if (from_definition && to_definition) {
            erase_by_from_definition(*from_definition);

            pairs_.emplace_back(*from_definition,
                                *to_definition);
          }
        }
      }

    } else if (json.is_object()) {
      // v1 format

      for (auto it = json.begin(); it != json.end(); ++it) {
        // it.key() is always std::string.
        if (it.value().is_string()) {
          definition from_definition("key_code", it.key());
          definition to_definition("key_code", it.value());

          erase_by_from_definition(from_definition);

          pairs_.emplace_back(from_definition,
                              to_definition);
        }
      }

    } else {
      logger::get_logger().error("json error: Invalid type: {0}", json.dump());
    }

    std::sort(pairs_.begin(), pairs_.end(), [](auto& a, auto& b) {
      return a.first.compare(b.first);
    });
  }

  void erase_by_from_definition(const definition& from_definition) {
    pairs_.erase(std::remove_if(std::begin(pairs_),
                                std::end(pairs_),
                                [&](auto& p) {
                                  return p.first == from_definition;
                                }),
                 std::end(pairs_));
  }

  std::vector<std::pair<definition, definition>> pairs_;
};
