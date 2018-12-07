#pragma once

#define ENQUEUE_EVENT(QUEUE, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT)                 \
  QUEUE.emplace_back_event(krbn::device_id(DEVICE_ID),                                                 \
                           krbn::event_queue::event_time_stamp(krbn::absolute_time_point(TIME_STAMP)), \
                           EVENT,                                                                      \
                           krbn::event_type::EVENT_TYPE,                                               \
                           ORIGINAL_EVENT)

#define PUSH_BACK_ENTRY(VECTOR, DEVICE_ID, TIME_STAMP, EVENT, EVENT_TYPE, ORIGINAL_EVENT)                               \
  VECTOR.push_back(krbn::event_queue::entry(krbn::device_id(DEVICE_ID),                                                 \
                                            krbn::event_queue::event_time_stamp(krbn::absolute_time_point(TIME_STAMP)), \
                                            EVENT,                                                                      \
                                            krbn::event_type::EVENT_TYPE,                                               \
                                            ORIGINAL_EVENT))
