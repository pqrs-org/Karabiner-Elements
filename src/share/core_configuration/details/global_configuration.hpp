#pragma once

#include <pqrs/json.hpp>

namespace krbn {
namespace core_configuration {
namespace details {
class global_configuration final {
public:
  global_configuration(const nlohmann::json& json) : json_(json),
                                                     check_for_updates_on_startup_(true),
                                                     show_in_menu_bar_(true),
                                                     show_profile_name_in_menu_bar_(false) {
    if (auto v = pqrs::json::find<bool>(json, "check_for_updates_on_startup")) {
      check_for_updates_on_startup_ = *v;
    }

    if (auto v = pqrs::json::find<bool>(json, "show_in_menu_bar")) {
      show_in_menu_bar_ = *v;
    }

    if (auto v = pqrs::json::find<bool>(json, "show_profile_name_in_menu_bar")) {
      show_profile_name_in_menu_bar_ = *v;
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["check_for_updates_on_startup"] = check_for_updates_on_startup_;
    j["show_in_menu_bar"] = show_in_menu_bar_;
    j["show_profile_name_in_menu_bar"] = show_profile_name_in_menu_bar_;
    return j;
  }

  bool get_check_for_updates_on_startup(void) const {
    return check_for_updates_on_startup_;
  }
  void set_check_for_updates_on_startup(bool value) {
    check_for_updates_on_startup_ = value;
  }

  bool get_show_in_menu_bar(void) const {
    return show_in_menu_bar_;
  }
  void set_show_in_menu_bar(bool value) {
    show_in_menu_bar_ = value;
  }

  bool get_show_profile_name_in_menu_bar(void) const {
    return show_profile_name_in_menu_bar_;
  }
  void set_show_profile_name_in_menu_bar(bool value) {
    show_profile_name_in_menu_bar_ = value;
  }

private:
  nlohmann::json json_;
  bool check_for_updates_on_startup_;
  bool show_in_menu_bar_;
  bool show_profile_name_in_menu_bar_;
};

inline void to_json(nlohmann::json& json, const global_configuration& global_configuration) {
  json = global_configuration.to_json();
}
} // namespace details
} // namespace core_configuration
} // namespace krbn
