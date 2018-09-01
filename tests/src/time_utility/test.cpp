#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "thread_utility.hpp"
#include "time_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("absolute_to_milliseconds") {
  {
    uint64_t ns = 256 * 1000000;
    auto absolute = krbn::time_utility::nano_to_absolute(ns);
    REQUIRE(krbn::time_utility::absolute_to_milliseconds(absolute) == std::chrono::milliseconds(256));
  }
}
