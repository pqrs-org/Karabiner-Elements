#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "time_utility.hpp"

TEST_CASE("time_utility::convert") {
  {
    std::chrono::nanoseconds ns(256 * 1000000);
    auto absolute_time_duration = krbn::time_utility::to_absolute_time_duration(ns);
    REQUIRE(krbn::time_utility::to_milliseconds(absolute_time_duration) == std::chrono::milliseconds(256));
  }

  {
    std::chrono::milliseconds ms(256 * 1000000);
    auto absolute_time_duration = krbn::time_utility::to_absolute_time_duration(ms);
    REQUIRE(krbn::time_utility::to_milliseconds(absolute_time_duration) == ms);
  }

  {
    krbn::absolute_time_duration absolute_time_duration(256 * 1000000);
    auto ns = krbn::time_utility::to_nanoseconds(absolute_time_duration);
    REQUIRE(krbn::time_utility::to_absolute_time_duration(ns) == absolute_time_duration);
  }

  {
    krbn::absolute_time_duration absolute_time_duration(256 * 1000000);
    auto ms = krbn::time_utility::to_milliseconds(absolute_time_duration);
    REQUIRE(krbn::time_utility::to_absolute_time_duration(ms) == absolute_time_duration);
  }
}
