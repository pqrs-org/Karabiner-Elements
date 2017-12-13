#pragma once

class virtual_hid_keyboard final {
public:
  virtual_hid_keyboard(const nlohmann::json& json) : json_(json),
                                                     caps_lock_delay_milliseconds_(0) {
    if (auto v = json_utility::find_optional<std::string>(json, "keyboard_type")) {
      keyboard_type_ = *v;
    }

    if (auto v = json_utility::find_optional<uint32_t>(json, "caps_lock_delay_milliseconds")) {
      caps_lock_delay_milliseconds_ = *v;
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["keyboard_type"] = keyboard_type_;
    j["caps_lock_delay_milliseconds"] = caps_lock_delay_milliseconds_;
    return j;
  }

  const std::string& get_keyboard_type(void) const {
    return keyboard_type_;
  }
  void set_keyboard_type(const std::string& value) {
    keyboard_type_ = value;
  }

  uint32_t get_caps_lock_delay_milliseconds(void) const {
    return caps_lock_delay_milliseconds_;
  }
  void set_caps_lock_delay_milliseconds(uint32_t value) {
    caps_lock_delay_milliseconds_ = value;
  }

  bool operator==(const virtual_hid_keyboard& other) const {
    return keyboard_type_ == other.keyboard_type_ &&
           caps_lock_delay_milliseconds_ == other.caps_lock_delay_milliseconds_;
  }

private:
  nlohmann::json json_;
  std::string keyboard_type_;
  uint32_t caps_lock_delay_milliseconds_;
};
