#pragma once

// `krbn::event_queue::entry` can be used safely in a multi-threaded environment.

#include "event.hpp"
#include "event_time_stamp.hpp"
#include "state.hpp"
#include "types.hpp"
#include <gsl/gsl>
#include <pqrs/json.hpp>

namespace krbn {
namespace event_queue {

class entry final {
public:
  // Constructors

  entry(device_id device_id,
        const event_time_stamp& event_time_stamp,
        const class event& event,
        event_type event_type,
        std::optional<event_integer_value::value_t> event_integer_value,
        const class event& original_event,
        state state,
        bool lazy = false,
        validity validity = validity::valid)
      : device_id_(device_id),
        event_time_stamp_(event_time_stamp),
        validity_(validity),
        state_(state),
        lazy_(lazy),
        event_(event),
        event_type_(event_type),
        event_integer_value_(event_integer_value),
        original_event_(original_event) {
  }

  entry& operator=(const entry& other) {
    device_id_ = other.device_id_;
    event_time_stamp_ = other.event_time_stamp_;
    validity_ = other.validity_;
    state_ = other.state_;
    lazy_ = other.lazy_;
    event_ = other.event_;
    event_type_ = other.event_type_;
    event_integer_value_ = other.event_integer_value_;
    original_event_ = other.original_event_;
    return *this;
  }

  entry(const entry& other) {
    *this = other;
  }

  static entry make_from_json(const nlohmann::json& json) {
    entry result(device_id(0),
                 event_time_stamp(absolute_time_point(0)),
                 event(),
                 event_type::key_down,
                 std::nullopt,
                 event(),
                 state::original);

    if (json.is_object()) {
      if (auto v = pqrs::json::find<uint32_t>(json, "device_id")) {
        result.device_id_ = device_id(*v);
      }

      if (auto v = pqrs::json::find_json(json, "event_time_stamp")) {
        result.event_time_stamp_ = event_time_stamp::make_from_json(v->value());
      }

      if (auto v = pqrs::json::find<int>(json, "validity")) {
        result.validity_ = static_cast<validity>(*v);
      }

      if (auto v = pqrs::json::find<state>(json, "state")) {
        result.state_ = *v;
      }

      if (auto v = pqrs::json::find<bool>(json, "lazy")) {
        result.lazy_ = *v;
      }

      if (auto v = pqrs::json::find_json(json, "event")) {
        result.event_ = event::make_from_json(v->value());
      }

      if (auto v = pqrs::json::find_json(json, "event_type")) {
        result.event_type_ = v->value().get<event_type>();
      }

      if (auto v = pqrs::json::find_json(json, "event_integer_value")) {
        result.event_integer_value_ = v->value().get<event_integer_value::value_t>();
      }

      if (auto v = pqrs::json::find_json(json, "original_event")) {
        result.original_event_ = event::make_from_json(v->value());
      }
    }

    return result;
  }

  // Methods

  device_id get_device_id(void) const {
    // We don't have to use mutex since there is not setter.

    return device_id_;
  }

  const event_time_stamp& get_event_time_stamp(void) const {
    // We don't have to use mutex since there is not setter.

    return event_time_stamp_;
  }

  event_time_stamp& get_event_time_stamp(void) {
    return const_cast<event_time_stamp&>(static_cast<const entry&>(*this).get_event_time_stamp());
  }

  validity get_validity(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return validity_;
  }

  void set_validity(validity value) {
    std::lock_guard<std::mutex> lock(mutex_);

    validity_ = value;
  }

  state get_state(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return state_;
  }

  void set_state(state value) {
    std::lock_guard<std::mutex> lock(mutex_);

    state_ = value;
  }

  bool get_lazy(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return lazy_;
  }

  void set_lazy(bool value) {
    std::lock_guard<std::mutex> lock(mutex_);

    lazy_ = value;
  }

  const event& get_event(void) const {
    // We don't have to use mutex since there is not setter.

    return event_;
  }

  event_type get_event_type(void) const {
    // We don't have to use mutex since there is not setter.

    return event_type_;
  }

  std::optional<event_integer_value::value_t> get_event_integer_value(void) const {
    // We don't have to use mutex since there is not setter.

    return event_integer_value_;
  }

  const event& get_original_event(void) const {
    // We don't have to use mutex since there is not setter.

    return original_event_;
  }

  nlohmann::json to_json(void) const {
    auto json = nlohmann::json({
        {"device_id", type_safe::get(get_device_id())},
        {"event_time_stamp", get_event_time_stamp()},
        {"validity", static_cast<int>(get_validity())},
        {"state", get_state()},
        {"lazy", get_lazy()},
        {"event", get_event()},
        {"event_type", get_event_type()},
        {"original_event", get_original_event()},
    });
    if (auto v = get_event_integer_value()) {
      json["event_integer_value"] = get_event_integer_value();
    }
    return json;
  }

  bool operator==(const entry& other) const {
    return get_device_id() == other.get_device_id() &&
           get_event_time_stamp() == other.get_event_time_stamp() &&
           get_validity() == other.get_validity() &&
           get_state() == other.get_state() &&
           get_lazy() == other.get_lazy() &&
           get_event() == other.get_event() &&
           get_event_type() == other.get_event_type() &&
           get_event_integer_value() == other.get_event_integer_value() &&
           get_original_event() == other.get_original_event();
  }

  bool operator!=(const entry& other) const {
    return !(*this == other);
  }

private:
  device_id device_id_;
  event_time_stamp event_time_stamp_;

  // Event will be marked invalid if it is changed by a manipulator.
  // The validity flag will be reset each following manipulation stage.
  //
  // - simple modifiactions
  // - complex modifications
  // - fn function keys
  // - post event to virtual devices
  //
  validity validity_;

  // Event will be marked as state::manipulated if the event is posted by manipulator at least once.
  // The state will be kept each above manipulation state.
  state state_;

  bool lazy_;
  event event_;
  event_type event_type_;

  // For events sent by devices that have pqrs::hid::usage::consumer::programmable_buttons,
  // the integer_value can have a meaningful value.
  // For example, with the VEC USB Footpedal INFINITY USB-3, all three foot switches send `button75`,
  // but the integer_value changes depending on which switches are pressed.
  // Specifically, each switch is assigned 1, 2, or 4,
  // and the integer_value becomes the sum of the numbers for the pressed foot switches.
  // We store the integer_value in event_integer_value_;
  std::optional<event_integer_value::value_t> event_integer_value_;

  event original_event_;
  mutable std::mutex mutex_;
};

inline void to_json(nlohmann::json& json, const entry& value) {
  json = value.to_json();
}

typedef pqrs::not_null_shared_ptr_t<const entry> not_null_const_entry_ptr_t;
typedef pqrs::not_null_shared_ptr_t<std::vector<not_null_const_entry_ptr_t>> not_null_entries_ptr_t;

} // namespace event_queue
} // namespace krbn
