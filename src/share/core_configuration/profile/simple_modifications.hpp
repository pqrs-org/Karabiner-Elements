#pragma once

class simple_modifications final {
public:
  class definition final {
  public:
    definition(const std::string& type,
               const std::string& value) : type_(type),
                                           value_(value) {
    }

    nlohmann::json to_json(void) const {
      return nlohmann::json({
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

  simple_modifications(const nlohmann::json& json) {
    if (json.is_object()) {
      for (auto it = json.begin(); it != json.end(); ++it) {
        // it.key() is always std::string.
        if (it.value().is_string()) {
          std::string value = it.value();
          pairs_.emplace_back(it.key(), value);
        }
      }

      std::sort(pairs_.begin(), pairs_.end(), [](const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b) {
        return SI::natural::compare<std::string>(a.first, b.first);
      });
    }
  }

  nlohmann::json to_json(void) const {
    auto json = nlohmann::json::object();
    for (const auto& it : pairs_) {
      if (!it.first.empty() &&
          !it.second.empty() &&
          json.find(it.first) == json.end()) {
        json[it.first] = it.second;
      }
    }
    return json;
  }

  const std::vector<std::pair<std::string, std::string>>& get_pairs(void) const {
    return pairs_;
  }

  void push_back_pair(void) {
    pairs_.emplace_back("", "");
  }

  void erase_pair(size_t index) {
    if (index < pairs_.size()) {
      pairs_.erase(pairs_.begin() + index);
    }
  }

  void replace_pair(size_t index, const std::string& from, const std::string& to) {
    if (index < pairs_.size()) {
      pairs_[index].first = from;
      pairs_[index].second = to;
    }
  }

  void replace_second(const std::string& from, const std::string& to) {
    for (auto&& it : pairs_) {
      if (it.first == from) {
        it.second = to;
        return;
      }
    }
  }

  std::unordered_map<key_code, key_code> make_key_code_map(void) const {
    std::unordered_map<key_code, key_code> map;

    for (const auto& it : pairs_) {
      auto& from_string = it.first;
      auto& to_string = it.second;

      if (to_string.empty()) {
        continue;
      }

      auto from_key_code = types::get_key_code(from_string);
      if (!from_key_code) {
        continue;
      }

      auto to_key_code = types::get_key_code(to_string);
      if (!to_key_code) {
        continue;
      }

      map[*from_key_code] = *to_key_code;
    }

    return map;
  }

private:
  std::vector<std::pair<std::string, std::string>> pairs_;
};
