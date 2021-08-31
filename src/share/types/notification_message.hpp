#pragma once

#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>

namespace krbn {
class notification_message final {
public:
  notification_message(void) {
  }

  const std::string& get_id(void) const {
    return id_;
  }

  void set_id(const std::string& value) {
    id_ = value;
  }

  const std::string& get_text(void) const {
    return text_;
  }

  void set_text(const std::string& text) {
    text_ = text;
  }

  bool operator==(const notification_message&) const = default;

private:
  std::string id_;
  std::string text_;
};

inline void to_json(nlohmann::json& json, const notification_message& value) {
  json = nlohmann::json::object({
      {"id", value.get_id()},
      {"text", value.get_text()},
  });
}

inline void from_json(const nlohmann::json& json, notification_message& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "id") {
      pqrs::json::requires_string(v, k);
      value.set_id(v.get<std::string>());
    } else if (k == "text") {
      pqrs::json::requires_string(v, k);
      value.set_text(v.get<std::string>());
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::notification_message> final {
  std::size_t operator()(const krbn::notification_message& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_id());
    pqrs::hash::combine(h, value.get_text());

    return h;
  }
};
} // namespace std
