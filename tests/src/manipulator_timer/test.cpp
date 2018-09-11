#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "manipulator/manipulator_timer.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("manipulator_timer") {
  std::cout << "manipulator_timer" << std::endl;

  auto manipulator_timer = std::make_unique<krbn::manipulator::manipulator_timer>(false);
  int client_object;
  auto client_id = krbn::manipulator::manipulator_timer::make_client_id(&client_object);
  std::vector<int> result;

  manipulator_timer->enqueue(client_id, [&] { result.push_back(1); }, krbn::absolute_time(3));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(2); }, krbn::absolute_time(2));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(3); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(4); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(5); }, krbn::absolute_time(2));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(6); }, krbn::absolute_time(3));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(7); }, krbn::absolute_time(2));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(8); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(9); }, krbn::absolute_time(3));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(10); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id, [&] { result.push_back(11); }, krbn::absolute_time(100));

  manipulator_timer->async_invoke(krbn::absolute_time(10));
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

TEST_CASE("manipulator_timer.enqueue_earlier") {
  std::cout << "manipulator_timer.enqueue_earlier" << std::endl;

  // Test with an actual timer.
  bool timer_enabled = true;
  auto manipulator_timer = std::make_unique<krbn::manipulator::manipulator_timer>(timer_enabled);
  int client_object;
  auto client_id = krbn::manipulator::manipulator_timer::make_client_id(&client_object);
  std::vector<int> result;
  std::mutex m;

  manipulator_timer->enqueue(client_id,
                             [&] { std::lock_guard<std::mutex> l(m); result.push_back(1); },
                             krbn::time_utility::to_absolute_time(std::chrono::milliseconds(1000)));
  manipulator_timer->async_invoke(krbn::time_utility::to_absolute_time(std::chrono::milliseconds(100)));

  manipulator_timer->enqueue(client_id,
                             [&] { std::lock_guard<std::mutex> l(m); result.push_back(2); },
                             krbn::time_utility::to_absolute_time(std::chrono::milliseconds(200)));
  manipulator_timer->async_invoke(krbn::time_utility::to_absolute_time(std::chrono::milliseconds(100)));

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  {
    std::lock_guard<std::mutex> l(m);

    REQUIRE(result.size() == 1);
    REQUIRE(result[0] == 2);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  {
    std::lock_guard<std::mutex> l(m);

    REQUIRE(result.size() == 2);
    REQUIRE(result[0] == 2);
    REQUIRE(result[1] == 1);
  }

  manipulator_timer = nullptr;
}

namespace {
bool check_thread_id(boost::optional<std::thread::id> thread_id) {
  if (!thread_id) {
    thread_id = std::this_thread::get_id();
  }
  return *thread_id == std::this_thread::get_id();
}
} // namespace

TEST_CASE("manipulator_timer.thread_id") {
  std::cout << "manipulator_timer.thread_id" << std::endl;

  auto manipulator_timer = std::make_unique<krbn::manipulator::manipulator_timer>(false);
  int client_object;
  auto client_id = krbn::manipulator::manipulator_timer::make_client_id(&client_object);

  boost::optional<std::thread::id> thread_id;

  manipulator_timer->enqueue(client_id, [&] { REQUIRE(check_thread_id(thread_id)); }, krbn::absolute_time(0));
  manipulator_timer->enqueue(client_id, [&] { REQUIRE(check_thread_id(thread_id)); }, krbn::absolute_time(5));
  manipulator_timer->enqueue(client_id, [&] { REQUIRE(check_thread_id(thread_id)); }, krbn::absolute_time(10));
  manipulator_timer->enqueue(client_id, [&] { REQUIRE(check_thread_id(thread_id)); }, krbn::absolute_time(15));

  manipulator_timer->async_invoke(krbn::absolute_time(10));
  manipulator_timer->async_invoke(krbn::absolute_time(20));
  manipulator_timer->async_invoke(krbn::absolute_time(100));

  manipulator_timer = nullptr;
}

TEST_CASE("manipulator_timer.async_erase") {
  auto manipulator_timer = std::make_unique<krbn::manipulator::manipulator_timer>(false);
  int client_object1;
  auto client_id1 = krbn::manipulator::manipulator_timer::make_client_id(&client_object1);
  int client_object2;
  auto client_id2 = krbn::manipulator::manipulator_timer::make_client_id(&client_object2);
  std::vector<int> result;

  manipulator_timer->enqueue(client_id1, [&] { result.push_back(1); }, krbn::absolute_time(3));
  manipulator_timer->enqueue(client_id1, [&] { result.push_back(2); }, krbn::absolute_time(2));
  manipulator_timer->enqueue(client_id1, [&] { result.push_back(3); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id1, [&] { result.push_back(4); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id1, [&] { result.push_back(5); }, krbn::absolute_time(2));
  manipulator_timer->enqueue(client_id2, [&] { result.push_back(6); }, krbn::absolute_time(3));
  manipulator_timer->enqueue(client_id2, [&] { result.push_back(7); }, krbn::absolute_time(2));
  manipulator_timer->enqueue(client_id2, [&] { result.push_back(8); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id2, [&] { result.push_back(9); }, krbn::absolute_time(3));
  manipulator_timer->enqueue(client_id2, [&] { result.push_back(10); }, krbn::absolute_time(1));
  manipulator_timer->enqueue(client_id2, [&] { result.push_back(11); }, krbn::absolute_time(100));

  krbn::thread_utility::wait wait;
  manipulator_timer->async_erase(client_id2,
                                 [&wait] {
                                   wait.notify();
                                 });
  manipulator_timer->async_invoke(krbn::absolute_time(10));
  wait.wait_notice();

  REQUIRE(result.size() == 5);
  REQUIRE(result[0] == 3);
  REQUIRE(result[1] == 4);
  REQUIRE(result[2] == 2);
  REQUIRE(result[3] == 5);
  REQUIRE(result[4] == 1);

  manipulator_timer = nullptr;
}
