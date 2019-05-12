#pragma once

#include "types.hpp"
#include <cstdint>
#include <ostream>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class grabbable_state final {
public:
  enum class state : uint32_t {
    none,
    grabbable,
    ungrabbable_temporarily,
    ungrabbable_permanently,
    device_error,
    end_,
  };

  grabbable_state(void) : grabbable_state(device_id(0),
                                          state::grabbable,
                                          absolute_time_point(0)) {
  }

  grabbable_state(device_id device_id,
                  state state,
                  absolute_time_point time_stamp) : device_id_(device_id),
                                                    state_(state),
                                                    time_stamp_(time_stamp) {
  }

  device_id get_device_id(void) const {
    return device_id_;
  }

  void set_device_id(device_id value) {
    device_id_ = value;
  }

  state get_state(void) const {
    return state_;
  }

  void set_state(state value) {
    state_ = value;
  }

  absolute_time_point get_time_stamp(void) const {
    return time_stamp_;
  }

  void set_time_stamp(absolute_time_point value) {
    time_stamp_ = value;
  }

  bool equals_except_time_stamp(const grabbable_state& other) const {
    return device_id_ == other.device_id_ &&
           state_ == other.state_;
  }

  bool operator==(const grabbable_state& other) const {
    return device_id_ == other.device_id_ &&
           state_ == other.state_ &&
           time_stamp_ == other.time_stamp_;
  }

  bool operator!=(const grabbable_state& other) const { return !(*this == other); }

private:
  device_id device_id_;
  state state_;
  absolute_time_point time_stamp_;
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    grabbable_state::state,
    {
        {grabbable_state::state::none, nullptr},
        {grabbable_state::state::grabbable, "grabbable"},
        {grabbable_state::state::ungrabbable_temporarily, "ungrabbable_temporarily"},
        {grabbable_state::state::ungrabbable_permanently, "ungrabbable_permanently"},
        {grabbable_state::state::device_error, "device_error"},
        {grabbable_state::state::end_, "end_"},
    });

inline void to_json(nlohmann::json& j, const grabbable_state& s) {
  j = nlohmann::json{
      {"device_id", s.get_device_id()},
      {"state", s.get_state()},
      {"time_stamp", s.get_time_stamp()},
  };
}

inline void from_json(const nlohmann::json& j, grabbable_state& s) {
  try {
    s.set_device_id(j.at("device_id").get<device_id>());
  } catch (...) {}

  try {
    s.set_state(j.at("state").get<grabbable_state::state>());
  } catch (...) {}

  try {
    s.set_time_stamp(j.at("time_stamp").get<absolute_time_point>());
  } catch (...) {}
}

inline std::ostream& operator<<(std::ostream& stream, const grabbable_state& value) {
  stream << nlohmann::json(value);
  return stream;
}
} // namespace krbn
