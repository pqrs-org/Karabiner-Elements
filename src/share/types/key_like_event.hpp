#pragma once

#include "consumer_key_code.hpp"
#include "key_code.hpp"
#include "pointing_button.hpp"
#include <mpark/variant.hpp>
#include <nlohmann/json.hpp>

namespace krbn {
class key_like_event final {
public:
  key_like_event(void) : value_(mpark::monostate()) {
  }

  template <typename T>
  key_like_event(T value) : value_(value) {
  }

  template <typename T>
  void set_value(T value) {
    value_ = value;
  }

  template <typename T>
  const T* find(void) const {
    return mpark::get_if<T>(&value_);
  }

  bool operator==(const key_like_event& other) const {
    return value_ == other.value_;
  }

  bool operator<(const key_like_event& other) const {
    return value_ < other.value_;
  }

private:
  mpark::variant<key_code,
                 consumer_key_code,
                 pointing_button,
                 mpark::monostate>
      value_;
};

inline void to_json(nlohmann::json& json, const key_like_event& value) {
  if (auto v = value.find<key_code>()) {
    json["key_code"] = *v;

  } else if (auto v = value.find<consumer_key_code>()) {
    json["consumer_key_code"] = *v;

  } else if (auto v = value.find<pointing_button>()) {
    json["pointing_button"] = *v;
  }
}

inline void from_json(const nlohmann::json& json, key_like_event& value) {
  if (!json.is_object()) {
    throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
  }

  for (const auto& [k, v] : json.items()) {
    if (k == "key_code") {
      value.set_value(v.get<key_code>());

    } else if (k == "consumer_key_code") {
      value.set_value(v.get<consumer_key_code>());

    } else if (k == "pointing_button") {
      value.set_value(v.get<pointing_button>());

    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}

} // namespace krbn
