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
  using value_t = mpark::variant<key_code::value_t,
                                 consumer_key_code::value_t,
                                 pointing_button::value_t,
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
    if (auto&& v = find<key_code::value_t>()) {
      if (auto&& m = make_modifier_flag(*v)) {
        return true;
      }
    }
    return false;
  }

  std::string to_string(void) const {
    if (auto value = find<key_code::value_t>()) {
      auto json = nlohmann::json::object({
          {"key_code", make_key_code_name(*value)},
      });
      return json.dump();

    } else if (auto value = find<consumer_key_code::value_t>()) {
      auto json = nlohmann::json::object({
          {"consumer_key_code", make_consumer_key_code_name(*value)},
      });
      return json.dump();

    } else if (auto value = find<pointing_button::value_t>()) {
      auto json = nlohmann::json::object({
          {"pointing_button", make_pointing_button_name(*value)},
      });
      return json.dump();
    }

    return "";
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
  if (auto v = value.find<key_code::value_t>()) {
    json["key_code"] = type_safe::get(*v);

  } else if (auto v = value.find<consumer_key_code::value_t>()) {
    json["consumer_key_code"] = type_safe::get(*v);

  } else if (auto v = value.find<pointing_button::value_t>()) {
    json["pointing_button"] = type_safe::get(*v);
  }
}

inline void from_json(const nlohmann::json& json, key_down_up_valued_event& value) {
  if (!json.is_object()) {
    throw pqrs::json::unmarshal_error(fmt::format("json must be object, but is `{0}`", json.dump()));
  }

  for (const auto& [k, v] : json.items()) {
    if (k == "key_code") {
      value.set_value(v.get<key_code::value_t>());

    } else if (k == "consumer_key_code") {
      value.set_value(v.get<consumer_key_code::value_t>());

    } else if (k == "pointing_button") {
      value.set_value(v.get<pointing_button::value_t>());

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
