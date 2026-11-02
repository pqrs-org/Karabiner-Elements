#pragma once

// `krbn::event_queue::entry` can be used safely in a multi-threaded environment.

#include "event.hpp"
#include "event_time_stamp.hpp"
#include "state.hpp"
#include "types.hpp"
#include <gsl/gsl>
#include <pqrs/json.hpp>
#include <set>
#include <unordered_set>

namespace krbn::event_queue {

class entry final {
public:
  struct parameters final {
    krbn::device_id device_id{0};
    krbn::event_queue::event_time_stamp event_time_stamp;
    krbn::event_queue::event event;
    krbn::event_type event_type{krbn::event_type::key_down};
    std::optional<event_integer_value::value_t> event_integer_value;
    krbn::event_queue::event original_event;
    krbn::event_queue::state state{krbn::event_queue::state::original};
    bool lazy{false};
    krbn::validity validity{krbn::validity::valid};
    bool output_only_modifier{false};
    std::unordered_set<modifier_flag> additional_modifiers_for_matching;
  };

  // Constructors

  explicit entry(const parameters& parameters)
      : device_id_(parameters.device_id),
        event_time_stamp_(parameters.event_time_stamp),
        validity_(parameters.validity),
        state_(parameters.state),
        lazy_(parameters.lazy),
        output_only_modifier_(parameters.output_only_modifier),
        additional_modifiers_for_matching_(parameters.additional_modifiers_for_matching),
        event_(parameters.event),
        event_type_(parameters.event_type),
        event_integer_value_(parameters.event_integer_value),
        original_event_(parameters.original_event) {
  }

  entry& operator=(const entry& other) {
    device_id_ = other.device_id_;
    event_time_stamp_ = other.event_time_stamp_;
    validity_ = other.validity_;
    state_ = other.state_;
    lazy_ = other.lazy_;
    output_only_modifier_ = other.output_only_modifier_;
    additional_modifiers_for_matching_ = other.additional_modifiers_for_matching_;
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
    entry result(parameters{});

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

      if (auto v = pqrs::json::find<bool>(json, "output_only_modifier")) {
        result.output_only_modifier_ = *v;
      }

      if (auto v = pqrs::json::find<std::unordered_set<modifier_flag>>(json, "additional_modifiers_for_matching")) {
        result.additional_modifiers_for_matching_ = *v;
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

  [[nodiscard]] device_id get_device_id() const {
    // We don't have to use mutex since there is not setter.

    return device_id_;
  }

  [[nodiscard]] const event_time_stamp& get_event_time_stamp() const {
    // We don't have to use mutex since there is not setter.

    return event_time_stamp_;
  }

  [[nodiscard]] event_time_stamp& get_event_time_stamp() {
    return const_cast<event_time_stamp&>(static_cast<const entry&>(*this).get_event_time_stamp());
  }

  [[nodiscard]] validity get_validity() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return validity_;
  }

  void set_validity(validity value) {
    std::lock_guard<std::mutex> lock(mutex_);

    validity_ = value;
  }

  [[nodiscard]] state get_state() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return state_;
  }

  void set_state(state value) {
    std::lock_guard<std::mutex> lock(mutex_);

    state_ = value;
  }

  [[nodiscard]] bool get_lazy() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return lazy_;
  }

  void set_lazy(bool value) {
    std::lock_guard<std::mutex> lock(mutex_);

    lazy_ = value;
  }

  [[nodiscard]] bool get_output_only_modifier() const {
    // We don't have to use mutex since there is no setter.

    return output_only_modifier_;
  }

  [[nodiscard]] const std::unordered_set<modifier_flag>& get_additional_modifiers_for_matching() const {
    return additional_modifiers_for_matching_;
  }

  [[nodiscard]] const event& get_event() const {
    // We don't have to use mutex since there is not setter.

    return event_;
  }

  [[nodiscard]] event_type get_event_type() const {
    // We don't have to use mutex since there is not setter.

    return event_type_;
  }

  [[nodiscard]] std::optional<event_integer_value::value_t> get_event_integer_value() const {
    // We don't have to use mutex since there is not setter.

    return event_integer_value_;
  }

  [[nodiscard]] const event& get_original_event() const {
    // We don't have to use mutex since there is not setter.

    return original_event_;
  }

  nlohmann::json to_json() const {
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
    if (get_output_only_modifier()) {
      json["output_only_modifier"] = true;
    }
    if (!get_additional_modifiers_for_matching().empty()) {
      json["additional_modifiers_for_matching"] = std::set<modifier_flag>(
          std::begin(get_additional_modifiers_for_matching()),
          std::end(get_additional_modifiers_for_matching()));
    }
    return json;
  }

  bool operator==(const entry& other) const {
    return get_device_id() == other.get_device_id() &&
           get_event_time_stamp() == other.get_event_time_stamp() &&
           get_validity() == other.get_validity() &&
           get_state() == other.get_state() &&
           get_lazy() == other.get_lazy() &&
           get_output_only_modifier() == other.get_output_only_modifier() &&
           get_additional_modifiers_for_matching() == other.get_additional_modifiers_for_matching() &&
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

  // If true, a modifier event updates the logical modifier state without
  // immediately dispatching it to the virtual device. The state is dispatched
  // when a following output event needs the current modifiers.
  bool lazy_;

  // True for generated modifiers that remain active only to preserve the
  // modifiers of a repeating output key. These modifiers are excluded from
  // subsequent `from.modifiers` matching.
  bool output_only_modifier_;

  // Modifiers generated as part of the same output operation as this event.
  // They participate in downstream matching for this event, even though they
  // are excluded from matching unrelated subsequent physical input.
  //
  // For example, `1 -> shift+1` keeps the generated shift pressed so that
  // macOS can repeat shift+1. The generated `1` entry stores shift here, which
  // allows a downstream `shift+1 -> b` rule to match it. Because shift is
  // attached only to that entry, a physical key pressed afterward is matched
  // without the generated shift.
  //
  // Without this entry-specific state, the generated shift would be excluded
  // from the modifier state used for matching, so the generated `1` would be
  // treated as an unmodified key and the downstream `shift+1 -> b` rule would
  // not match.
  std::unordered_set<modifier_flag> additional_modifiers_for_matching_;

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

} // namespace krbn::event_queue
