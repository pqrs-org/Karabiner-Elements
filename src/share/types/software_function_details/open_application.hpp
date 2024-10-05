#pragma once

#include <CoreGraphics/CoreGraphics.h>
#include <optional>
#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <regex>

namespace krbn {
namespace software_function_details {
class open_application {
public:
  open_application(void) {
  }

  const std::optional<std::string>& get_bundle_identifier(void) const {
    return bundle_identifier_;
  }

  void set_bundle_identifier(const std::optional<std::string>& value) {
    bundle_identifier_ = value;
  }

  const std::optional<std::string>& get_file_path(void) const {
    return file_path_;
  }

  void set_file_path(const std::optional<std::string>& value) {
    file_path_ = value;
  }

  constexpr bool operator==(const open_application&) const = default;

private:
  std::optional<std::string> bundle_identifier_;
  std::optional<std::string> file_path_;
};

inline void to_json(nlohmann::json& json, const open_application& value) {
  if (auto v = value.get_bundle_identifier()) {
    json["bundle_identifier"] = *v;
  }
  if (auto v = value.get_file_path()) {
    json["file_path"] = *v;
  }
}

inline void from_json(const nlohmann::json& json, open_application& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "bundle_identifier") {
      value.set_bundle_identifier(v.get<std::string>());
    } else if (k == "file_path") {
      value.set_file_path(v.get<std::string>());
    }
  }
}
} // namespace software_function_details
} // namespace krbn

namespace std {
template <>
struct hash<krbn::software_function_details::open_application> final {
  std::size_t operator()(const krbn::software_function_details::open_application& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_bundle_identifier());
    pqrs::hash::combine(h, value.get_file_path());

    return h;
  }
};
} // namespace std
