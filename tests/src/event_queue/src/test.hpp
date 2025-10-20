#pragma once

#include "event_queue.hpp"

inline std::optional<krbn::event_integer_value::value_t> make_event_integer_value(krbn::event_type type) {
  if (type == krbn::event_type::key_down) {
    return krbn::event_integer_value::value_t(1);
  } else if (type == krbn::event_type::key_up) {
    return krbn::event_integer_value::value_t(0);
  }
  return std::nullopt;
}

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT)                 \
  QUEUE.emplace_back_entry(krbn::device_id(DEVICE_ID),                                                 \
                           krbn::event_queue::event_time_stamp(krbn::absolute_time_point(TIME_STAMP)), \
                           EVENT,                                                                      \
                           krbn::event_type::EVENT_TYPE,                                               \
                           make_event_integer_value(krbn::event_type::EVENT_TYPE),                     \
                           ORIGINAL_EVENT,                                                             \
                           krbn::event_queue::state::original)

#define PUSH_BACK_ENTRY(VECTOR, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT)                               \
  VECTOR.push_back(krbn::event_queue::entry(krbn::device_id(DEVICE_ID),                                                 \
                                            krbn::event_queue::event_time_stamp(krbn::absolute_time_point(TIME_STAMP)), \
                                            EVENT,                                                                      \
                                            krbn::event_type::EVENT_TYPE,                                               \
                                            make_event_integer_value(krbn::event_type::EVENT_TYPE),                     \
                                            ORIGINAL_EVENT,                                                             \
                                            krbn::event_queue::state::original))

#define PUSH_BACK_ENTRY_PTR(VECTOR, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT)                                             \
  VECTOR.push_back(std::make_shared<krbn::event_queue::entry>(krbn::device_id(DEVICE_ID),                                                 \
                                                              krbn::event_queue::event_time_stamp(krbn::absolute_time_point(TIME_STAMP)), \
                                                              EVENT,                                                                      \
                                                              krbn::event_type::EVENT_TYPE,                                               \
                                                              make_event_integer_value(krbn::event_type::EVENT_TYPE),                     \
                                                              ORIGINAL_EVENT,                                                             \
                                                              krbn::event_queue::state::original))
