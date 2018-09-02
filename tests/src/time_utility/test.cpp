#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"
#include "time_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("absolute_to_milliseconds") {
  {
    std::chrono::nanoseconds ns(256 * 1000000);
    auto absolute = krbn::time_utility::nanoseconds_to_absolute(ns);
    REQUIRE(krbn::time_utility::absolute_to_milliseconds(absolute) == std::chrono::milliseconds(256));
  }
}
