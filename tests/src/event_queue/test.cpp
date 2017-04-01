#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "event_queue.hpp"
#include "thread_utility.hpp"

TEST_CASE("push_back_event") {
  // normal order
  {
    krbn::manipulator::event_queue event_queue;

    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(100),
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(200),
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(300),
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_up);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(400),
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_up);

    REQUIRE(event_queue.get_input_events() == std::vector<krbn::manipulator::event_queue::queued_event>({
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(100),
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(200),
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(300),
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_up),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(400),
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_up),
                                              }));
  }

  // reorder events
  {
    krbn::manipulator::event_queue event_queue;

    // push "a" and "left_shift" (key_down) at the same time.
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(100),
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(100),
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(200),
                                *(krbn::types::get_key_code("spacebar")),
                                krbn::event_type::key_down);
    // push "a" and "left_shift" (key_up) at the same time.
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(300),
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_up);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                std::chrono::nanoseconds(300),
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_up);

    REQUIRE(event_queue.get_input_events() == std::vector<krbn::manipulator::event_queue::queued_event>({
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(100),
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(100),
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(200),
                                                                                               *(krbn::types::get_key_code("spacebar")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(300),
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_up),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               std::chrono::nanoseconds(300),
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_up),

                                              }));
  }
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
