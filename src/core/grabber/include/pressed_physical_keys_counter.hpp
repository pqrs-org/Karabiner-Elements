#pragma once

#include "boost_defs.hpp"

#include "types.hpp"
#include <boost/optional.hpp>
#include <vector>

namespace krbn {
class pressed_physical_keys_counter final {
public:
  class pressed_key final {
  public:
    enum class type {
      key_code,
      pointing_button,
    };

    pressed_key(device_id device_id,
                key_code key_code) : device_id_(device_id),
                                     type_(type::key_code),
                                     key_code_(key_code) {
    }

    pressed_key(device_id device_id,
                pointing_button pointing_button) : device_id_(device_id),
                                                   type_(type::pointing_button),
                                                   pointing_button_(pointing_button) {
    }

    device_id get_device_id(void) const {
      return device_id_;
    }

    type get_type(void) const {
      return type_;
    }

    boost::optional<key_code> get_key_code(void) const {
      if (type_ == type::key_code) {
        return key_code_;
      }
      return boost::none;
    }

    boost::optional<pointing_button> get_pointing_button(void) const {
      if (type_ == type::pointing_button) {
        return pointing_button_;
      }
      return boost::none;
    }

    bool operator==(const pressed_key& other) const {
      return get_device_id() == other.get_device_id() &&
             get_type() == other.get_type() &&
             get_key_code() == other.get_key_code() &&
             get_pointing_button() == other.get_pointing_button();
    }

  private:
    device_id device_id_;
    type type_;
    union {
      key_code key_code_;
      pointing_button pointing_button_;
    };
  };

  bool empty(device_id device_id) {
    for (const auto& k : pressed_keys_) {
      if (k.get_device_id() == device_id) {
        return false;
      }
    }
    return true;
  }

  bool is_pointing_button_pressed(device_id device_id) {
    for (const auto& k : pressed_keys_) {
      if (k.get_device_id() == device_id &&
          k.get_type() == pressed_key::type::pointing_button) {
        return true;
      }
    }
    return false;
  }

  bool update(device_id device_id,
              hid_usage_page usage_page,
              hid_usage usage,
              CFIndex integer_value) {
    if (auto key_code = types::get_key_code(usage_page, usage)) {
      if (integer_value) {
        emplace_back_pressed_key(device_id, *key_code);
      } else {
        erase_all_pressed_keys(device_id, *key_code);
      }
      return true;
    }

    if (auto pointing_button = types::get_pointing_button(usage_page, usage)) {
      if (integer_value) {
        emplace_back_pressed_key(device_id, *pointing_button);
      } else {
        erase_all_pressed_keys(device_id, *pointing_button);
      }
      return true;
    }

    return false;
  }

  void emplace_back_pressed_key(device_id device_id,
                                key_code key_code) {
    pressed_keys_.emplace_back(device_id, key_code);
  }

  void emplace_back_pressed_key(device_id device_id,
                                pointing_button pointing_button) {
    pressed_keys_.emplace_back(device_id, pointing_button);
  }

  void erase_all_pressed_keys(device_id device_id) {
    pressed_keys_.erase(std::remove_if(std::begin(pressed_keys_),
                                       std::end(pressed_keys_),
                                       [&](const pressed_key& k) {
                                         return k.get_device_id() == device_id;
                                       }),
                        std::end(pressed_keys_));
  }

  void erase_all_pressed_keys(device_id device_id,
                              key_code key_code) {
    pressed_keys_.erase(std::remove_if(std::begin(pressed_keys_),
                                       std::end(pressed_keys_),
                                       [&](const pressed_key& k) {
                                         return k.get_device_id() == device_id &&
                                                k.get_key_code() == key_code;
                                       }),
                        std::end(pressed_keys_));
  }

  void erase_all_pressed_keys(device_id device_id,
                              pointing_button pointing_button) {
    pressed_keys_.erase(std::remove_if(std::begin(pressed_keys_),
                                       std::end(pressed_keys_),
                                       [&](const pressed_key& k) {
                                         return k.get_device_id() == device_id &&
                                                k.get_pointing_button() == pointing_button;
                                       }),
                        std::end(pressed_keys_));
  }

private:
  std::vector<pressed_key> pressed_keys_;
};
} // namespace krbn
