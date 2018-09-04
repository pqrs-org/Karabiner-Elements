#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "manipulator/manipulator_timer.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("manipulator_timer") {
  auto manipulator_timer = std::make_unique<krbn::manipulator::manipulator_timer>();
  std::vector<int> result;

  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(1); }, krbn::absolute_time(3)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(2); }, krbn::absolute_time(2)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(3); }, krbn::absolute_time(1)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(4); }, krbn::absolute_time(1)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(5); }, krbn::absolute_time(2)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(6); }, krbn::absolute_time(3)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(7); }, krbn::absolute_time(2)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(8); }, krbn::absolute_time(1)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(9); }, krbn::absolute_time(3)),
                             krbn::absolute_time(0));
  manipulator_timer->enqueue(krbn::manipulator::manipulator_timer::entry(
                                 [&] { result.push_back(10); }, krbn::absolute_time(1)),
                             krbn::absolute_time(0));

  manipulator_timer->async_signal(krbn::absolute_time(10000));
  manipulator_timer = nullptr;

  REQUIRE(result.size() == 10);
  REQUIRE(result[0] == 3);
  REQUIRE(result[1] == 4);
  REQUIRE(result[2] == 8);
  REQUIRE(result[3] == 10);
  REQUIRE(result[4] == 2);
  REQUIRE(result[5] == 5);
  REQUIRE(result[6] == 7);
  REQUIRE(result[7] == 1);
  REQUIRE(result[8] == 6);
  REQUIRE(result[9] == 9);
}
