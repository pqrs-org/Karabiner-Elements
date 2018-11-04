#pragma once

// `krbn::event_queue::event_time_stamp` can be used safely in a multi-threaded environment.

#include "time_utility.hpp"
#include "types.hpp"
#include <nlohmann/json.hpp>
#include <ostream>

namespace krbn {
namespace event_queue {
class event_time_stamp final {
public:
  // Constructors

  event_time_stamp(void) : event_time_stamp(absolute_time_point(0)) {
  }

  event_time_stamp(absolute_time_point time_stamp) : time_stamp_(time_stamp),
                                                     input_delay_duration_(0) {
  }

  event_time_stamp(absolute_time_point time_stamp,
                   absolute_time_duration input_delay_duration) : time_stamp_(time_stamp),
                                                                  input_delay_duration_(input_delay_duration) {
  }

  event_time_stamp& operator=(const event_time_stamp& other) {
    time_stamp_ = other.get_time_stamp();
    input_delay_duration_ = other.get_input_delay_duration();
    return *this;
  }

  event_time_stamp(const event_time_stamp& other) {
    *this = other;
  }

  static event_time_stamp make_from_json(const nlohmann::json& json) {
    event_time_stamp result(absolute_time_point(0),
                            absolute_time_duration(0));

    if (auto v = json_utility::find_optional<uint64_t>(json, "time_stamp")) {
      result.time_stamp_ += time_utility::to_absolute_time_duration(std::chrono::milliseconds(*v));
    }

    if (auto v = json_utility::find_optional<uint64_t>(json, "input_delay_duration")) {
      result.input_delay_duration_ += time_utility::to_absolute_time_duration(std::chrono::milliseconds(*v));
    }

    return result;
  }

  // Methods

  absolute_time_point get_time_stamp(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return time_stamp_;
  }

  void set_time_stamp(absolute_time_point value) {
    std::lock_guard<std::mutex> lock(mutex_);

    time_stamp_ = value;
  }

  absolute_time_duration get_input_delay_duration(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return input_delay_duration_;
  }

  void set_input_delay_duration(absolute_time_duration value) {
    std::lock_guard<std::mutex> lock(mutex_);

    input_delay_duration_ = value;
  }

  absolute_time_point make_time_stamp_with_input_delay(void) const {
    std::lock_guard<std::mutex> lock(mutex_);

    return time_stamp_ + input_delay_duration_;
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json::object({
        {"time_stamp", time_utility::to_milliseconds(get_time_stamp() - absolute_time_point(0)).count()},
        {"input_delay_duration", time_utility::to_milliseconds(get_input_delay_duration()).count()},
    });
  }

  bool operator==(const event_time_stamp& other) const {
    return get_time_stamp() == other.get_time_stamp() &&
           get_input_delay_duration() == other.get_input_delay_duration();
  }

  bool operator!=(const event_time_stamp& other) const {
    return !(*this == other);
  }

  friend size_t hash_value(const event_time_stamp& value) {
    size_t h = 0;
    boost::hash_combine(h, type_safe::get(value.get_time_stamp()));
    boost::hash_combine(h, type_safe::get(value.get_input_delay_duration()));
    return h;
  }

private:
  absolute_time_point time_stamp_;
  absolute_time_duration input_delay_duration_;
  mutable std::mutex mutex_;
};

inline std::ostream& operator<<(std::ostream& stream, const event_time_stamp& value) {
  stream << std::endl
         << "{"
         << "\"time_stamp\":" << value.get_time_stamp()
         << ",\"input_delay_duration\":" << value.get_input_delay_duration()
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
