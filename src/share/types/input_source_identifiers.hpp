#pragma once

#include "boost_defs.hpp"

#include "types/product_id.hpp"
#include "types/vendor_id.hpp"
#include <boost/functional/hash.hpp>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/osx/input_source.hpp>

namespace krbn {
class input_source_identifiers final {
public:
  input_source_identifiers(void) {
  }

  input_source_identifiers(TISInputSourceRef p) : language_(pqrs::osx::input_source::make_first_language(p)),
                                                  input_source_id_(pqrs::osx::input_source::make_input_source_id(p)),
                                                  input_mode_id_(pqrs::osx::input_source::make_input_mode_id(p)) {
  }

  input_source_identifiers(const std::optional<std::string>& language,
                           const std::optional<std::string>& input_source_id,
                           const std::optional<std::string>& input_mode_id) : language_(language),
                                                                              input_source_id_(input_source_id),
                                                                              input_mode_id_(input_mode_id) {
  }

  input_source_identifiers(const nlohmann::json& json) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "language") {
          if (value.is_string()) {
            language_ = value.get<std::string>();
          } else {
            logger::get_logger()->error("language should be string: {0}", json.dump());
          }
        } else if (key == "input_source_id") {
          if (value.is_string()) {
            input_source_id_ = value.get<std::string>();
          } else {
            logger::get_logger()->error("input_source_id should be string: {0}", json.dump());
          }
        } else if (key == "input_mode_id") {
          if (value.is_string()) {
            input_mode_id_ = value.get<std::string>();
          } else {
            logger::get_logger()->error("input_mode_id should be string: {0}", json.dump());
          }
        } else {
          logger::get_logger()->error("json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    } else {
      logger::get_logger()->error("input_source_identifiers should be object: {0}", json.dump());
    }
  }

  nlohmann::json to_json(void) const {
    auto json = nlohmann::json::object();

    if (language_) {
      json["language"] = *language_;
    }

    if (input_source_id_) {
      json["input_source_id"] = *input_source_id_;
    }

    if (input_mode_id_) {
      json["input_mode_id"] = *input_mode_id_;
    }

    return json;
  }

  const std::optional<std::string>& get_language(void) const {
    return language_;
  }

  const std::optional<std::string>& get_input_source_id(void) const {
    return input_source_id_;
  }

  const std::optional<std::string>& get_input_mode_id(void) const {
    return input_mode_id_;
  }

  bool operator==(const input_source_identifiers& other) const {
    return language_ == other.language_ &&
           input_source_id_ == other.input_source_id_ &&
           input_mode_id_ == other.input_mode_id_;
  }

  friend size_t hash_value(const input_source_identifiers& value) {
    size_t h = 0;
    if (value.language_) {
      boost::hash_combine(h, *(value.language_));
    }
    if (value.input_source_id_) {
      boost::hash_combine(h, *(value.input_source_id_));
    }
    if (value.input_mode_id_) {
      boost::hash_combine(h, *(value.input_mode_id_));
    }
    return h;
  }

private:
  std::optional<std::string> language_;
  std::optional<std::string> input_source_id_;
  std::optional<std::string> input_mode_id_;
};

inline std::ostream& operator<<(std::ostream& stream, const input_source_identifiers& value) {
  stream << "language:";

  if (auto& v = value.get_language()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_source_id:";

  if (auto& v = value.get_input_source_id()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_mode_id:";

  if (auto& v = value.get_input_mode_id()) {
    stream << *v;
  } else {
    stream << "---";
  }

  return stream;
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::input_source_identifiers> final {
  std::size_t operator()(const krbn::input_source_identifiers& v) const {
    return hash_value(v);
  }
};
} // namespace std
