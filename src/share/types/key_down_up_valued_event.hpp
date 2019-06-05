#pragma once

#include "consumer_key_code.hpp"
#include "key_code.hpp"
#include "pointing_button.hpp"
#include <mpark/variant.hpp>
#include <nlohmann/json.hpp>
#include <pqrs/hash.hpp>

namespace krbn {
class key_down_up_valued_event final {
public:
  using value_t = mpark::variant<key_code,
                                 consumer_key_code,
                                 pointing_button,
                                 mpark::monostate>;

  key_down_up_valued_event(void) : value_(mpark::monostate()) {
  }

  template <typename T>
  explicit key_down_up_valued_event(T value) : value_(value) {
  }

  const value_t& get_value(void) const {
    return value_;
  }

  template <typename T>
  void set_value(T value) {
    value_ = value;
  }

  template <typename T>
  const T* find(void) const {
    return mpark::get_if<T>(&value_);
  }

  bool modifier_flag(void) const {
    if (auto&& v = find<key_code>()) {
      if (auto&& m = make_modifier_flag(*v)) {
        return true;
      }
    }
    return false;
  }

  bool operator==(const key_down_up_valued_event& other) const {
    return value_ == other.value_;
  }

  bool operator<(const key_down_up_valued_event& other) const {
    return value_ < other.value_;
  }

private:
  value_t value_;
};

inline void to_json(nlohmann::json& json, const key_down_up_valued_event& value) {
  if (auto v = value.find<key_code>()) {
    json["key_code"] = *v;

  } else if (auto v = value.find<consumer_key_code>()) {
    json["consumer_key_code"] = *v;

  } else if (auto v = value.find<pointing_button>()) {
    json["pointing_button"] = *v;
  }
}

inline void from_json(const nlohmann::json& json, key_down_up_valued_event& value) {
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

namespace std {
template <>
struct hash<krbn::key_down_up_valued_event> final {
  std::size_t operator()(const krbn::key_down_up_valued_event& value) const {
    std::size_t h = 0;

    pqrs::hash_combine(h, value.get_value());

    return h;
  }
};
} // namespace std
