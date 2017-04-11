#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "manipulator/event_queue.hpp"
#include "thread_utility.hpp"

TEST_CASE("push_back_event") {
  // normal order
  {
    krbn::manipulator::event_queue event_queue;

    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                100,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                200,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                300,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_up);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                400,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_up);

    REQUIRE(event_queue.get_input_events() == std::vector<krbn::manipulator::event_queue::queued_event>({
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               100,
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               200,
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               300,
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_up),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               400,
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_up),
                                              }));

    REQUIRE(event_queue.get_input_events()[0].get_valid() == true);
    REQUIRE(event_queue.get_input_events()[0].get_lazy() == false);
  }

  // reorder events
  {
    krbn::manipulator::event_queue event_queue;

    // push "a" and "left_shift" (key_down) at the same time.
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                100,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                100,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_down);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                200,
                                *(krbn::types::get_key_code("spacebar")),
                                krbn::event_type::key_down);
    // push "a" and "left_shift" (key_up) at the same time.
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                300,
                                *(krbn::types::get_key_code("left_shift")),
                                krbn::event_type::key_up);
    event_queue.push_back_event(krbn::manipulator::event_queue::scope::input,
                                300,
                                *(krbn::types::get_key_code("a")),
                                krbn::event_type::key_up);

    REQUIRE(event_queue.get_input_events() == std::vector<krbn::manipulator::event_queue::queued_event>({
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               100,
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               100,
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               200,
                                                                                               *(krbn::types::get_key_code("spacebar")),
                                                                                               krbn::event_type::key_down),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               300,
                                                                                               *(krbn::types::get_key_code("a")),
                                                                                               krbn::event_type::key_up),
                                                  krbn::manipulator::event_queue::queued_event(krbn::manipulator::event_queue::scope::input,
                                                                                               300,
                                                                                               *(krbn::types::get_key_code("left_shift")),
                                                                                               krbn::event_type::key_up),

                                              }));
  }
}

TEST_CASE("compare") {
  krbn::manipulator::event_queue::queued_event spacebar_down(krbn::manipulator::event_queue::scope::input,
                                                             100,
                                                             *(krbn::types::get_key_code("spacebar")),
                                                             krbn::event_type::key_down);
  krbn::manipulator::event_queue::queued_event right_shift_down(krbn::manipulator::event_queue::scope::input,
                                                                100,
                                                                *(krbn::types::get_key_code("right_shift")),
                                                                krbn::event_type::key_down);
  krbn::manipulator::event_queue::queued_event escape_down(krbn::manipulator::event_queue::scope::input,
                                                           200,
                                                           *(krbn::types::get_key_code("escape")),
                                                           krbn::event_type::key_down);
  krbn::manipulator::event_queue::queued_event spacebar_up(krbn::manipulator::event_queue::scope::input,
                                                             300,
                                                             *(krbn::types::get_key_code("spacebar")),
                                                             krbn::event_type::key_up);
  krbn::manipulator::event_queue::queued_event right_shift_up(krbn::manipulator::event_queue::scope::input,
                                                                300,
                                                                *(krbn::types::get_key_code("right_shift")),
                                                                krbn::event_type::key_up);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, spacebar_down) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, escape_down) == true);
  REQUIRE(krbn::manipulator::event_queue::compare(escape_down, spacebar_down) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, right_shift_down) == false);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_down, spacebar_down) == true);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_down, right_shift_up) == true);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_up, spacebar_down) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_up, right_shift_up) == true);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_up, spacebar_up) == false);

  REQUIRE(krbn::manipulator::event_queue::compare(spacebar_up, right_shift_down) == false);
  REQUIRE(krbn::manipulator::event_queue::compare(right_shift_down, spacebar_up) == true);
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
