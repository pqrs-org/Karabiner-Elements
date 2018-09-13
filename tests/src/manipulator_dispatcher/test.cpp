#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "manipulator/manipulator_dispatcher.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("manipulator_dispatcher") {
  std::cout << "manipulator_dispatcher" << std::endl;

  auto manipulator_dispatcher = std::make_unique<krbn::manipulator::manipulator_dispatcher>();
  auto manipulator_object_id = krbn::manipulator::make_new_manipulator_object_id();
  std::vector<int> result;

  manipulator_dispatcher->async_attach(manipulator_object_id);

  manipulator_dispatcher->enqueue(manipulator_object_id, [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result.push_back(1);
  });
  manipulator_dispatcher->enqueue(manipulator_object_id, [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result.push_back(2);
  });
  manipulator_dispatcher->enqueue(manipulator_object_id, [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result.push_back(3);
  });
  manipulator_dispatcher->enqueue(manipulator_object_id, [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result.push_back(4);
  });
  manipulator_dispatcher->enqueue(manipulator_object_id, [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result.push_back(5);
  });

  REQUIRE(result.size() == 0);

  manipulator_dispatcher->detach(manipulator_object_id);

  REQUIRE(result.size() == 5);

  // Ignored after `detach`

  manipulator_dispatcher->enqueue(manipulator_object_id, [&] {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    result.push_back(6);
  });

  manipulator_dispatcher = nullptr;

  REQUIRE(result.size() == 5);
}
