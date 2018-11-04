#pragma once

#include "json_utility.hpp"
#include <natural_sort.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
using namespace std::string_literals;

class simple_modifications final {
public:
  simple_modifications(const nlohmann::json& json) {
    handle_json(json);
  }

  nlohmann::json to_json(void) const {
    std::unordered_set<std::string> froms;

    auto json = nlohmann::json::array();
    for (const auto& it : pairs_) {
      try {
        auto from_json = nlohmann::json::parse(it.first);
        auto to_json = nlohmann::json::parse(it.second);

        if (from_json.is_object() &&
            to_json.is_object()) {
          if (!from_json.empty() &&
              !to_json.empty() &&
              froms.find(it.first) == std::end(froms)) {
            json.push_back(nlohmann::json{
                {"from", from_json},
                {"to", to_json},
            });
            froms.insert(it.first);
          }
        }
      } catch (std::exception&) {
      }
    }
    return json;
  }

  const std::vector<std::pair<std::string, std::string>>& get_pairs(void) const {
    return pairs_;
  }

  void update(const nlohmann::json& json) {
    handle_json(json);
  }

  void push_back_pair(void) {
    pairs_.emplace_back(nlohmann::json::object().dump(),
                        nlohmann::json::object().dump());
  }

  void erase_pair(size_t index) {
    if (index < pairs_.size()) {
      pairs_.erase(pairs_.begin() + index);
    }
  }

  void replace_pair(size_t index,
                    const std::string& from,
                    const std::string& to) {
    try {
      auto from_json_string = nlohmann::json::parse(from).dump();
      auto to_json_string = nlohmann::json::parse(to).dump();

      if (index < pairs_.size()) {
        pairs_[index].first = from_json_string;
        pairs_[index].second = to_json_string;
      }
    } catch (std::exception&) {
    }
  }

  void replace_second(const std::string& from,
                      const std::string& to) {
    try {
      auto from_json_string = nlohmann::json::parse(from).dump();
      auto to_json_string = nlohmann::json::parse(to).dump();

      for (auto&& it : pairs_) {
        if (it.first == from_json_string) {
          it.second = to_json_string;
          return;
        }
      }
    } catch (std::exception&) {
    }
  }

private:
  void handle_json(const nlohmann::json& json) {
    // Override existing definition

    if (json.is_array()) {
      // v2 format

      for (const auto& j : json) {
        if (j.is_object()) {
          boost::optional<std::string> from_json_string;
          boost::optional<std::string> to_json_string;

          for (auto it = j.begin(); it != j.end(); ++it) {
            // it.key() is always std::string.
            const auto& key = it.key();
            const auto& value = it.value();

            if (key == "from") {
              nlohmann::json from_json = value;
              from_json.erase("");
              from_json_string = from_json.dump();
            } else if (key == "to") {
              nlohmann::json to_json = value;
              to_json.erase("");
              to_json_string = to_json.dump();
            } else {
              logger::get_logger().error("json error: Unknown key: {0} in {1}", key, j.dump());
            }
          }

          if (from_json_string && to_json_string) {
            erase_by_from_json_string(*from_json_string);

            pairs_.emplace_back(*from_json_string,
                                *to_json_string);
          }
        }
      }

    } else if (json.is_object()) {
      // v1 format

      for (auto it = json.begin(); it != json.end(); ++it) {
        // it.key() is always std::string.
        if (it.value().is_string()) {
          auto from_json_string = "{}"s;
          auto to_json_string = "{}"s;

          if (!it.key().empty()) {
            nlohmann::json from_json({{"key_code", it.key()}});
            from_json_string = from_json.dump();
          }

          if (!it.value().empty()) {
            nlohmann::json to_json({{"key_code", it.value()}});
            to_json_string = to_json.dump();
          }

          erase_by_from_json_string(from_json_string);

          pairs_.emplace_back(from_json_string,
                              to_json_string);
        }
      }

    } else {
      logger::get_logger().error("json error: Invalid type: {0}", json.dump());
    }

    std::sort(pairs_.begin(), pairs_.end(), [](auto& a, auto& b) {
      return SI::natural::compare<std::string>(a.first, b.first);
    });
  }

  void erase_by_from_json_string(const std::string& from) {
    try {
      auto from_json_string = nlohmann::json::parse(from).dump();

      pairs_.erase(std::remove_if(std::begin(pairs_),
                                  std::end(pairs_),
                                  [&](auto& p) {
                                    return p.first == from_json_string;
                                  }),
                   std::end(pairs_));
    } catch (std::exception&) {
    }
  }

  // Note:
  // std::string is form of nlohmann::json::dump() result.

  std::vector<std::pair<std::string, std::string>> pairs_;
};

inline void to_json(nlohmann::json& json, const simple_modifications& simple_modifications) {
  json = simple_modifications.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
