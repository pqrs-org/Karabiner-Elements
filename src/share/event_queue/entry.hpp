#pragma once

#include "event_queue/event.hpp"
#include "event_queue/event_time_stamp.hpp"
#include "json_utility.hpp"
#include "types.hpp"

namespace krbn {
namespace event_queue {
class entry final {
public:
  entry(device_id device_id,
        const event_time_stamp& event_time_stamp,
        const class event& event,
        event_type event_type,
        const class event& original_event,
        bool lazy = false) : device_id_(device_id),
                             event_time_stamp_(event_time_stamp),
                             valid_(true),
                             lazy_(lazy),
                             event_(event),
                             event_type_(event_type),
                             original_event_(original_event) {
  }

  static entry make_from_json(const nlohmann::json& json) {
    entry result(device_id::zero,
                 event_time_stamp(absolute_time(0)),
                 event(),
                 event_type::key_down,
                 event());

    if (json.is_object()) {
      if (auto v = json_utility::find_optional<uint32_t>(json, "device_id")) {
        result.device_id_ = device_id(*v);
      }

      if (auto v = json_utility::find_json(json, "event_time_stamp")) {
        result.event_time_stamp_ = event_time_stamp::make_from_json(*v);
      }

      if (auto v = json_utility::find_optional<bool>(json, "valid")) {
        result.valid_ = *v;
      }

      if (auto v = json_utility::find_optional<bool>(json, "lazy")) {
        result.lazy_ = *v;
      }

      if (auto v = json_utility::find_json(json, "event")) {
        result.event_ = event::make_from_json(*v);
      }

      if (auto v = json_utility::find_json(json, "event_type")) {
        result.event_type_ = *v;
      }

      if (auto v = json_utility::find_json(json, "original_event")) {
        result.original_event_ = event::make_from_json(*v);
      }
    }

    return result;
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"device_id", static_cast<uint32_t>(device_id_)},
        {"event_time_stamp", event_time_stamp_},
        {"valid", valid_},
        {"lazy", lazy_},
        {"event", event_},
        {"event_type", event_type_},
        {"original_event", original_event_},
    });
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  const event_time_stamp& get_event_time_stamp(void) const {
    return event_time_stamp_;
  }
  event_time_stamp& get_event_time_stamp(void) {
    return event_time_stamp_;
  }

  bool get_valid(void) const {
    return valid_;
  }
  void set_valid(bool value) {
    valid_ = value;
  }

  bool get_lazy(void) const {
    return lazy_;
  }
  void set_lazy(bool value) {
    lazy_ = value;
  }

  const event& get_event(void) const {
    return event_;
  }

  event_type get_event_type(void) const {
    return event_type_;
  }

  const event& get_original_event(void) const {
    return original_event_;
  }

  bool operator==(const entry& other) const {
    return get_device_id() == other.get_device_id() &&
           get_event_time_stamp() == other.get_event_time_stamp() &&
           get_valid() == other.get_valid() &&
           get_lazy() == other.get_lazy() &&
           get_event() == other.get_event() &&
           get_event_type() == other.get_event_type() &&
           get_original_event() == other.get_original_event();
  }

private:
  device_id device_id_;
  event_time_stamp event_time_stamp_;
  bool valid_;
  bool lazy_;
  event event_;
  event_type event_type_;
  event original_event_;
};

inline std::ostream& operator<<(std::ostream& stream, const entry& value) {
  stream << std::endl
         << "{"
         << "\"device_id\":" << value.get_device_id()
         << ",\"event_time_stamp\":" << value.get_event_time_stamp()
         << ",\"valid\":" << value.get_valid()
         << ",\"lazy\":" << value.get_lazy()
         << ",\"event\":" << value.get_event()
         << ",\"event_type\":" << value.get_event_type()
         << ",\"original_event\":" << value.get_original_event()
         << "}";
  return stream;
}

inline void to_json(nlohmann::json& json, const entry& value) {
  json = value.to_json();
}
} // namespace event_queue
} // namespace krbn
