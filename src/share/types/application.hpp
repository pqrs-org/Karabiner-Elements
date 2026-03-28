#pragma once

#include <optional>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <string>
#include <type_traits>

namespace krbn {
class application final {
public:
  enum class detection_source {
    none,
    workspace,
    ax_observer,
  };

  application() {
  }

  const std::optional<std::string>& get_bundle_identifier() const {
    return bundle_identifier_;
  }

  application& set_bundle_identifier(const std::optional<std::string>& value) {
    bundle_identifier_ = value;
    return *this;
  }

  const std::optional<std::string>& get_bundle_path() const {
    return bundle_path_;
  }

  application& set_bundle_path(const std::optional<std::string>& value) {
    bundle_path_ = value;
    return *this;
  }

  const std::optional<std::string>& get_file_path() const {
    return file_path_;
  }

  application& set_file_path(const std::optional<std::string>& value) {
    file_path_ = value;
    return *this;
  }

  const std::optional<pid_t>& get_pid() const {
    return pid_;
  }

  application& set_pid(const std::optional<pid_t>& value) {
    pid_ = value;
    return *this;
  }

  detection_source get_detection_source() const {
    return detection_source_;
  }

  application& set_detection_source(detection_source value) {
    detection_source_ = value;
    return *this;
  }

  bool operator==(const application& other) const = default;

private:
  std::optional<std::string> bundle_identifier_;
  std::optional<std::string> bundle_path_;
  std::optional<std::string> file_path_;
  std::optional<pid_t> pid_;
  detection_source detection_source_ = detection_source::none;
};

inline std::string to_string(application::detection_source value) {
  switch (value) {
    case application::detection_source::none:
      return "none";
    case application::detection_source::workspace:
      return "workspace";
    case application::detection_source::ax_observer:
      return "ax_observer";
  }

  throw pqrs::json::unmarshal_error("invalid detection_source");
}

inline application::detection_source detection_source_from_string(const std::string& value) {
  if (value == "none") {
    return application::detection_source::none;
  }

  if (value == "workspace") {
    return application::detection_source::workspace;
  }

  if (value == "ax_observer") {
    return application::detection_source::ax_observer;
  }

  throw pqrs::json::unmarshal_error("unknown detection_source: `" + value + "`");
}

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

  j["detection_source"] = to_string(s.get_detection_source());
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

    } else if (key == "detection_source") {
      pqrs::json::requires_string(value, "`"s + key + "`");

      s.set_detection_source(detection_source_from_string(value.get<std::string>()));

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

    pqrs::hash::combine(h, static_cast<std::underlying_type_t<krbn::application::detection_source>>(value.get_detection_source()));

    return h;
  }
};
} // namespace std
