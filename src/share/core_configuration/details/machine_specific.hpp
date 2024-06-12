#pragma once

#include "../configuration_json_helper.hpp"
#include <pqrs/json.hpp>
#include <unordered_map>

namespace krbn {
namespace core_configuration {
namespace details {
class machine_specific final {
public:
  class entry final {
  public:
    static constexpr bool enable_multitouch_extension_default_value = false;

    entry(const entry&) = delete;

    entry(const nlohmann::json& json)
        : json_(json),
          enable_multitouch_extension_(enable_multitouch_extension_default_value) {
      helper_values_.push_back_value<bool>("enable_multitouch_extension",
                                           enable_multitouch_extension_,
                                           enable_multitouch_extension_default_value);

      pqrs::json::requires_object(json, "json");

      helper_values_.update_value(json);
    }

    nlohmann::json to_json(void) const {
      auto j = json_;

      helper_values_.update_json(j);

      return j;
    }

    bool get_enable_multitouch_extension(void) const {
      return enable_multitouch_extension_;
    }
    void set_enable_multitouch_extension(bool value) {
      enable_multitouch_extension_ = value;
    }

  private:
    nlohmann::json json_;
    bool enable_multitouch_extension_;
    configuration_json_helper::helper_values helper_values_;
  };

  machine_specific(const machine_specific&) = delete;

  machine_specific(const nlohmann::json& json)
      : json_(json) {
    pqrs::json::requires_object(json, "json");

    for (const auto& [key, value] : json.items()) {
      if (value.is_object()) {
        entries_[karabiner_machine_identifier(key)] = std::make_shared<entry>(value);
      }
    }
  }

  nlohmann::json to_json(void) const {
    auto json = json_;

    for (const auto& [key, value] : entries_) {
      auto j = value->to_json();
      if (!j.empty()) {
        json[type_safe::get(key)] = j;
      } else {
        json.erase(type_safe::get(key));
      }
    }

    return json;
  }

  entry& get_entry(const karabiner_machine_identifier& identifier = constants::get_karabiner_machine_identifier()) {
    if (!entries_.contains(identifier)) {
      entries_[identifier] = std::make_shared<entry>(nlohmann::json::object());
    }

    return *(entries_[identifier]);
  }

private:
  nlohmann::json json_;
  std::unordered_map<karabiner_machine_identifier, std::shared_ptr<entry>> entries_;
};

inline void to_json(nlohmann::json& json, const machine_specific& value) {
  json = value.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
