#pragma once

#include <optional>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <string>

namespace krbn {
class application final {
public:
  application(void) {
  }

  const std::optional<std::string>& get_bundle_identifier(void) const {
    return bundle_identifier_;
  }

  application& set_bundle_identifier(const std::optional<std::string>& value) {
    bundle_identifier_ = value;
    return *this;
  }

  const std::optional<std::string>& get_bundle_path(void) const {
    return bundle_path_;
  }

  application& set_bundle_path(const std::optional<std::string>& value) {
    bundle_path_ = value;
    return *this;
  }

  const std::optional<std::string>& get_file_path(void) const {
    return file_path_;
  }

  application& set_file_path(const std::optional<std::string>& value) {
    file_path_ = value;
    return *this;
  }

  const std::optional<pid_t>& get_pid(void) const {
    return pid_;
  }

  application& set_pid(const std::optional<pid_t>& value) {
    pid_ = value;
    return *this;
  }

  bool operator==(const application& other) const = default;

private:
  std::optional<std::string> bundle_identifier_;
  std::optional<std::string> bundle_path_;
  std::optional<std::string> file_path_;
  std::optional<pid_t> pid_;
};

inline void to_json(nlohmann::json& j, const application& s) {
  j = nlohmann::json::object();

  if (auto& v = s.get_bundle_identifier()) {
    j["bundle_identifier"] = *v;
  }

  if (auto& v = s.get_bundle_path()) {
    j["bundle_path"] = *v;
  }

  if (auto& v = s.get_file_path()) {
    j["file_path"] = *v;
  }

  if (auto& v = s.get_pid()) {
    j["pid"] = *v;
  }
}

inline void from_json(const nlohmann::json& j, application& s) {
  using namespace std::string_literals;

  pqrs::json::requires_object(j, "json");

  for (const auto& [key, value] : j.items()) {
    if (key == "bundle_identifier") {
      pqrs::json::requires_string(value, "`"s + key + "`");

      s.set_bundle_identifier(value.get<std::string>());

    } else if (key == "bundle_path") {
      pqrs::json::requires_string(value, "`"s + key + "`");

      s.set_bundle_path(value.get<std::string>());

    } else if (key == "file_path") {
      pqrs::json::requires_string(value, "`"s + key + "`");

      s.set_file_path(value.get<std::string>());

    } else if (key == "pid") {
      pqrs::json::requires_number(value, "`"s + key + "`");

      s.set_pid(value.get<pid_t>());

    } else {
      throw pqrs::json::unmarshal_error("unknown key: `"s + key + "`"s);
    }
  }
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::application> final {
  std::size_t operator()(const krbn::application& value) const {
    size_t h = 0;

    if (auto& bundle_identifier = value.get_bundle_identifier()) {
      pqrs::hash::combine(h, *bundle_identifier);
    } else {
      pqrs::hash::combine(h, 0);
    }

    if (auto& bundle_path = value.get_bundle_path()) {
      pqrs::hash::combine(h, *bundle_path);
    } else {
      pqrs::hash::combine(h, 0);
    }

    if (auto& file_path = value.get_file_path()) {
      pqrs::hash::combine(h, *file_path);
    } else {
      pqrs::hash::combine(h, 0);
    }

    if (auto& pid = value.get_pid()) {
      pqrs::hash::combine(h, *pid);
    } else {
      pqrs::hash::combine(h, 0);
    }

    return h;
  }
};
} // namespace std
