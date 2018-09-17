#pragma once

#include "types.hpp"
#include <json/json.hpp>
#include <ostream>

namespace krbn {
namespace event_queue {
class event_time_stamp final {
public:
  explicit event_time_stamp(absolute_time time_stamp) : time_stamp_(time_stamp),
                                                        input_delay_time_stamp_(0) {
  }

  event_time_stamp(absolute_time time_stamp,
                   absolute_time input_delay_time_stamp) : time_stamp_(time_stamp),
                                                           input_delay_time_stamp_(input_delay_time_stamp) {
  }

  static event_time_stamp make_from_json(const nlohmann::json& json) {
    event_time_stamp result(absolute_time(0),
                            absolute_time(0));

    if (auto v = json_utility::find_optional<uint64_t>(json, "time_stamp")) {
      result.time_stamp_ = absolute_time(*v);
    }

    if (auto v = json_utility::find_optional<uint64_t>(json, "input_delay_time_stamp")) {
      result.input_delay_time_stamp_ = absolute_time(*v);
    }

    return result;
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json::object({
        {"time_stamp", type_safe::get(time_stamp_)},
        {"input_delay_time_stamp", type_safe::get(input_delay_time_stamp_)},
    });
  }

  absolute_time get_time_stamp(void) const {
    return time_stamp_;
  }

  void set_time_stamp(absolute_time value) {
    time_stamp_ = value;
  }

  absolute_time get_input_delay_time_stamp(void) const {
    return input_delay_time_stamp_;
  }

  void set_input_delay_time_stamp(absolute_time value) {
    input_delay_time_stamp_ = value;
  }

  absolute_time make_time_stamp_with_input_delay(void) const {
    return time_stamp_ + input_delay_time_stamp_;
  }

  bool operator==(const event_time_stamp& other) const {
    return time_stamp_ == other.time_stamp_ &&
           input_delay_time_stamp_ == other.input_delay_time_stamp_;
  }

  friend size_t hash_value(const event_time_stamp& value) {
    size_t h = 0;
    boost::hash_combine(h, type_safe::get(value.time_stamp_));
    boost::hash_combine(h, type_safe::get(value.input_delay_time_stamp_));
    return h;
  }

private:
  absolute_time time_stamp_;
  absolute_time input_delay_time_stamp_;
};

inline std::ostream& operator<<(std::ostream& stream, const event_time_stamp& value) {
  stream << std::endl
         << "{"
         << "\"time_stamp\":" << value.get_time_stamp()
         << ",\"input_delay_time_stamp\":" << value.get_input_delay_time_stamp()
         << "}";
  return stream;
}

inline void to_json(nlohmann::json& json, const event_time_stamp& value) {
  json = value.to_json();
}
} // namespace event_queue
} // namespace krbn

namespace std {
template <>
struct hash<krbn::event_queue::event_time_stamp> final {
  std::size_t operator()(const krbn::event_queue::event_time_stamp& v) const {
    return hash_value(v);
  }
};
} // namespace std
