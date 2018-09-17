#pragma once

#include <cstdint>
#include <ostream>
#include <type_safe/strong_typedef.hpp>

namespace krbn {
class input_source_selector final {
public:
  input_source_selector(const boost::optional<std::string>& language_string,
                        const boost::optional<std::string>& input_source_id_string,
                        const boost::optional<std::string>& input_mode_id_string) : language_string_(language_string),
                                                                                    input_source_id_string_(input_source_id_string),
                                                                                    input_mode_id_string_(input_mode_id_string) {
    update_regexs();
  }

  input_source_selector(const nlohmann::json& json) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "language") {
          if (value.is_string()) {
            language_string_ = value.get<std::string>();
          } else {
            logger::get_logger().error("complex_modifications json error: input_source_selector.language should be string: {0}", json.dump());
          }
        } else if (key == "input_source_id") {
          if (value.is_string()) {
            input_source_id_string_ = value.get<std::string>();
          } else {
            logger::get_logger().error("complex_modifications json error: input_source_selector.input_source_id should be string: {0}", json.dump());
          }
        } else if (key == "input_mode_id") {
          if (value.is_string()) {
            input_mode_id_string_ = value.get<std::string>();
          } else {
            logger::get_logger().error("complex_modifications json error: input_source_selector.input_mode_id should be string: {0}", json.dump());
          }
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    } else {
      logger::get_logger().error("complex_modifications json error: input_source_selector should be array of object: {0}", json.dump());
    }

    update_regexs();
  }

  nlohmann::json to_json(void) const {
    auto json = nlohmann::json::object();

    if (language_string_) {
      json["language"] = *language_string_;
    }

    if (input_source_id_string_) {
      json["input_source_id"] = *input_source_id_string_;
    }

    if (input_mode_id_string_) {
      json["input_mode_id"] = *input_mode_id_string_;
    }

    return json;
  }

  const boost::optional<std::string>& get_language_string(void) const {
    return language_string_;
  }

  const boost::optional<std::string>& get_input_source_id_string(void) const {
    return input_source_id_string_;
  }

  const boost::optional<std::string>& get_input_mode_id_string(void) const {
    return input_mode_id_string_;
  }

  bool test(const input_source_identifiers& input_source_identifiers) const {
    if (language_regex_) {
      if (auto& v = input_source_identifiers.get_language()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *language_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    if (input_source_id_regex_) {
      if (auto& v = input_source_identifiers.get_input_source_id()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *input_source_id_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    if (input_mode_id_regex_) {
      if (auto& v = input_source_identifiers.get_input_mode_id()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *input_mode_id_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    return true;
  }

  bool operator==(const input_source_selector& other) const {
    return language_string_ == other.language_string_ &&
           input_source_id_string_ == other.input_source_id_string_ &&
           input_mode_id_string_ == other.input_mode_id_string_;
  }

  friend size_t hash_value(const input_source_selector& value) {
    size_t h = 0;
    if (value.language_string_) {
      boost::hash_combine(h, *(value.language_string_));
    }
    if (value.input_source_id_string_) {
      boost::hash_combine(h, *(value.input_source_id_string_));
    }
    if (value.input_mode_id_string_) {
      boost::hash_combine(h, *(value.input_mode_id_string_));
    }

    // We can skip *_regex_ since *_regex_ is synchronized with *_string_.

    return h;
  }

private:
  void update_regexs(void) {
    std::string s;

    try {
      if (language_string_) {
        s = *language_string_;
        language_regex_ = std::regex(s);
      }

      if (input_source_id_string_) {
        s = *input_source_id_string_;
        input_source_id_regex_ = std::regex(s);
      }

      if (input_mode_id_string_) {
        s = *input_mode_id_string_;
        input_mode_id_regex_ = std::regex(s);
      }
    } catch (std::exception& e) {
      logger::get_logger().error("complex_modifications json error: Regex error: \"{0}\" {1}", s, e.what());
    }
  }

  boost::optional<std::string> language_string_;
  boost::optional<std::string> input_source_id_string_;
  boost::optional<std::string> input_mode_id_string_;

  boost::optional<std::regex> language_regex_;
  boost::optional<std::regex> input_source_id_regex_;
  boost::optional<std::regex> input_mode_id_regex_;
};

inline std::ostream& operator<<(std::ostream& stream, const input_source_selector& value) {
  stream << "language:";

  if (auto& v = value.get_language_string()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_source_id:";

  if (auto& v = value.get_input_source_id_string()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_mode_id:";

  if (auto& v = value.get_input_mode_id_string()) {
    stream << *v;
  } else {
    stream << "---";
  }

  return stream;
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<input_source_selector, std::allocator<input_source_selector>>& values) {
  bool first = true;
  stream << "[";
  for (const auto& v : values) {
    if (first) {
      first = false;
    } else {
      stream << ",";
    }
    stream << v;
  }
  stream << "]";
  return stream;
}

inline void to_json(nlohmann::json& json, const input_source_selector& input_source_selector) {
  json = input_source_selector.to_json();
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::input_source_selector> final {
  std::size_t operator()(const krbn::input_source_selector& v) const {
    return hash_value(v);
  }
};
} // namespace std
