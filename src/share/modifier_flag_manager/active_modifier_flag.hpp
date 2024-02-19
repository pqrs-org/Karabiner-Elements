#pragma once

class active_modifier_flag final {
public:
  enum class type {
    increase,
    decrease,
    increase_lock,
    decrease_lock,
    increase_led_lock, // Synchronized with LED state such as caps lock.
    decrease_led_lock, // Synchronized with LED state such as caps lock.
    increase_sticky,
    decrease_sticky,
  };

  active_modifier_flag(type type,
                       modifier_flag modifier_flag,
                       device_id device_id) : type_(type),
                                              modifier_flag_(modifier_flag),
                                              device_id_(device_id) {
    switch (type) {
      case type::increase:
      case type::decrease:
      case type::increase_lock:
      case type::decrease_lock:
      case type::increase_sticky:
      case type::decrease_sticky:
        break;

      case type::increase_led_lock:
      case type::decrease_led_lock:
        // The led_lock is shared by all devices because it is not sent from hardware.
        //
        // Note:
        // The caps lock state refers the virtual keyboard LED state and
        // it is not related with the caps lock key_down and key_up events.
        // (The behavior is described in docs/DEVELOPMENT.md.)

        device_id_ = krbn::device_id(0);
        break;
    }
  }

  type get_type(void) const {
    return type_;
  }

  modifier_flag get_modifier_flag(void) const {
    return modifier_flag_;
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  type get_inverse_type(void) const {
    switch (type_) {
      case type::increase:
        return type::decrease;
      case type::decrease:
        return type::increase;

      case type::increase_lock:
        return type::decrease_lock;
      case type::decrease_lock:
        return type::increase_lock;

      case type::increase_led_lock:
        return type::decrease_led_lock;
      case type::decrease_led_lock:
        return type::increase_led_lock;

      case type::increase_sticky:
        return type::decrease_sticky;
      case type::decrease_sticky:
        return type::increase_sticky;
    }
  }

  int get_count(void) const {
    switch (type_) {
      case type::increase:
      case type::increase_lock:
      case type::increase_led_lock:
      case type::increase_sticky:
        return 1;
      case type::decrease:
      case type::decrease_lock:
      case type::decrease_led_lock:
      case type::decrease_sticky:
        return -1;
    }
  }

  bool any_lock(void) const {
    switch (type_) {
      case type::increase_lock:
      case type::decrease_lock:
      case type::increase_led_lock:
      case type::decrease_led_lock:
        return true;

      case type::increase:
      case type::decrease:
      case type::increase_sticky:
      case type::decrease_sticky:
        return false;
    }
  }

  bool led_lock(void) const {
    switch (type_) {
      case type::increase_led_lock:
      case type::decrease_led_lock:
        return true;

      case type::increase:
      case type::decrease:
      case type::increase_lock:
      case type::decrease_lock:
      case type::increase_sticky:
      case type::decrease_sticky:
        return false;
    }
  }

  bool sticky(void) const {
    switch (type_) {
      case type::increase_sticky:
      case type::decrease_sticky:
        return true;

      case type::increase:
      case type::decrease:
      case type::increase_lock:
      case type::decrease_lock:
      case type::increase_led_lock:
      case type::decrease_led_lock:
        return false;
    }
  }

  bool is_paired(const active_modifier_flag& other) const {
    return get_type() == other.get_inverse_type() &&
           get_modifier_flag() == other.get_modifier_flag() &&
           get_device_id() == other.get_device_id();
  }

  constexpr auto operator<=>(const active_modifier_flag&) const = default;

  nlohmann::json to_json(void) const {
    nlohmann::json json;

    switch (type_) {
      case type::increase:
        json["type"] = "increase";
        break;
      case type::decrease:
        json["type"] = "decrease";
        break;
      case type::increase_lock:
        json["type"] = "increase_lock";
        break;
      case type::decrease_lock:
        json["type"] = "decrease_lock";
        break;
      case type::increase_led_lock:
        json["type"] = "increase_led_lock";
        break;
      case type::decrease_led_lock:
        json["type"] = "decrease_led_lock";
        break;
      case type::increase_sticky:
        json["type"] = "increase_sticky";
        break;
      case type::decrease_sticky:
        json["type"] = "decrease_sticky";
        break;
    }

    json["modifier_flag"] = modifier_flag_;
    json["device_id"] = device_id_;

    return json;
  }

private:
  type type_;
  modifier_flag modifier_flag_;
  device_id device_id_;
};
