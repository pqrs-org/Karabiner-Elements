#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "async_sequential_dispatcher.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("async_sequential_dispatcher") {
  int expected = 10;

  int actual = 0;
  krbn::async_sequential_dispatcher<int> dispatcher([&](auto&& i) {
    actual += i;
  });
  dispatcher.push_back(std::make_shared<int>(1));
  dispatcher.push_back(std::make_shared<int>(2));
  dispatcher.push_back(std::make_shared<int>(3));
  dispatcher.push_back(std::make_shared<int>(4));
  dispatcher.wait();

  REQUIRE(actual == expected);
}
