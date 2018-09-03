#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "manipulator/manipulator_timer.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("manipulator_timer") {
  auto manipulator_timer = std::make_unique<krbn::manipulator::manipulator_timer>();
  std::vector<krbn::manipulator::manipulator_timer::timer_id> timer_ids;

  manipulator_timer->timer_invoked.connect([&](auto&& entry) {
    timer_ids.push_back(entry.get_timer_id());
  });

  krbn::manipulator::manipulator_timer::entry entry1(krbn::absolute_time(1234));
  krbn::manipulator::manipulator_timer::entry entry2(krbn::absolute_time(1234));
  krbn::manipulator::manipulator_timer::entry entry3(krbn::absolute_time(5678));
  krbn::manipulator::manipulator_timer::entry entry4(krbn::absolute_time(5678));
  krbn::manipulator::manipulator_timer::entry entry5(krbn::absolute_time(2345));
  krbn::manipulator::manipulator_timer::entry entry6(krbn::absolute_time(2345));

  manipulator_timer->enqueue(entry1, krbn::absolute_time(0));
  manipulator_timer->enqueue(entry2, krbn::absolute_time(0));
  manipulator_timer->enqueue(entry3, krbn::absolute_time(0));
  manipulator_timer->enqueue(entry4, krbn::absolute_time(0));
  manipulator_timer->enqueue(entry5, krbn::absolute_time(0));
  manipulator_timer->enqueue(entry6, krbn::absolute_time(0));

  manipulator_timer->async_signal(krbn::absolute_time(10000));
  manipulator_timer = nullptr;

  REQUIRE(timer_ids.size() == 6);
  REQUIRE(timer_ids[0] == entry1.get_timer_id());
  REQUIRE(timer_ids[1] == entry2.get_timer_id());
  REQUIRE(timer_ids[2] == entry5.get_timer_id());
  REQUIRE(timer_ids[3] == entry6.get_timer_id());
  REQUIRE(timer_ids[4] == entry3.get_timer_id());
  REQUIRE(timer_ids[5] == entry4.get_timer_id());
}
