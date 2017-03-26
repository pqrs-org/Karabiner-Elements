#pragma once

class global_configuration final {
public:
  global_configuration(const nlohmann::json& json) : json_(json),
                                                     check_for_updates_on_startup_(true),
                                                     show_in_menu_bar_(true),
                                                     show_profile_name_in_menu_bar_(false) {
    {
      const std::string key = "check_for_updates_on_startup";
      if (json.find(key) != json.end() && json[key].is_boolean()) {
        check_for_updates_on_startup_ = json[key];
      }
    }
    {
      const std::string key = "show_in_menu_bar";
      if (json.find(key) != json.end() && json[key].is_boolean()) {
        show_in_menu_bar_ = json[key];
      }
    }
    {
      const std::string key = "show_profile_name_in_menu_bar";
      if (json.find(key) != json.end() && json[key].is_boolean()) {
        show_profile_name_in_menu_bar_ = json[key];
      }
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
