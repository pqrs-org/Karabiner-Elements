#pragma once

#include <chrono>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>

namespace krbn {
class notification_message final {
public:
  notification_message() {
  }

  [[nodiscard]] const std::string& get_id() const {
    return id_;
  }

  void set_id(const std::string& value) {
    id_ = value;
  }

  [[nodiscard]] const std::string& get_text() const {
    return text_;
  }

  void set_text(const std::string& text) {
    text_ = text;
  }

  [[nodiscard]] std::chrono::milliseconds get_duration_milliseconds() const {
    return duration_milliseconds_;
  }

  void set_duration_milliseconds(std::chrono::milliseconds value) {
    duration_milliseconds_ = value;
  }

  bool operator==(const notification_message&) const = default;

private:
  std::string id_;
  std::string text_;
  std::chrono::milliseconds duration_milliseconds_{0};
};

inline void to_json(nlohmann::json& json, const notification_message& value) {
  json = nlohmann::json::object({
      {"id", value.get_id()},
      {"text", value.get_text()},
  });

  if (value.get_duration_milliseconds().count() > 0) {
    json["duration_milliseconds"] = value.get_duration_milliseconds().count();
  }
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
    } else if (k == "duration_milliseconds") {
      pqrs::json::requires_number(v, "`" + k + "`");

      auto duration_milliseconds = v.get<int>();
      if (duration_milliseconds < 0) {
        throw pqrs::json::unmarshal_error(fmt::format("`{0}` must be greater than or equal to 0, but is `{1}`",
                                                      k,
                                                      duration_milliseconds));
      }

      value.set_duration_milliseconds(std::chrono::milliseconds(duration_milliseconds));
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
    pqrs::hash::combine(h, value.get_duration_milliseconds().count());

    return h;
  }
};
} // namespace std
