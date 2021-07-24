#pragma once

#include <pqrs/hash.hpp>
#include <pqrs/json.hpp>
#include <string_view>

namespace krbn {
namespace software_function_details {
class cg_event_double_click {
public:
  cg_event_double_click(void)
      : button_(0) {
  }

  uint32_t get_button(void) const {
    return button_;
  }

  void set_button(uint32_t value) {
    button_ = value;
  }

  constexpr bool operator==(const cg_event_double_click&) const = default;

private:
  uint32_t button_;
};

inline void to_json(nlohmann::json& json, const cg_event_double_click& value) {
  json["button"] = value.get_button();
}

inline void from_json(const nlohmann::json& json, cg_event_double_click& value) {
  pqrs::json::requires_object(json, "json");

  for (const auto& [k, v] : json.items()) {
    if (k == "button") {
      pqrs::json::requires_number(v, "`" + k + "`");
      value.set_button(v.get<int>());
    } else {
      throw pqrs::json::unmarshal_error(fmt::format("unknown key: `{0}`", k));
    }
  }
}
} // namespace software_function_details
} // namespace krbn

namespace std {
template <>
struct hash<krbn::software_function_details::cg_event_double_click> final {
  std::size_t operator()(const krbn::software_function_details::cg_event_double_click& value) const {
    std::size_t h = 0;

    pqrs::hash::combine(h, value.get_button());

    return h;
  }
};
} // namespace std
