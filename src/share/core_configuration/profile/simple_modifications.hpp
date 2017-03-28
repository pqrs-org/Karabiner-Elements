#pragma once

class simple_modifications final {
public:
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

  std::unordered_map<key_code, key_code> to_key_code_map(spdlog::logger& logger) const {
    std::unordered_map<key_code, key_code> map;

    for (const auto& it : pairs_) {
      auto& from_string = it.first;
      auto& to_string = it.second;

      auto from_key_code = types::get_key_code(from_string);
      if (!from_key_code) {
        logger.warn("unknown key_code:{0}", from_string);
        continue;
      }

      auto to_key_code = types::get_key_code(to_string);
      if (!to_key_code) {
        logger.warn("unknown key_code:{0}", to_string);
        continue;
      }

      map[*from_key_code] = *to_key_code;
    }

    return map;
  }

private:
  std::vector<std::pair<std::string, std::string>> pairs_;
};
