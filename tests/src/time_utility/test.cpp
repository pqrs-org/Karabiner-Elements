#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "time_utility.hpp"

TEST_CASE("time_utility::convert") {
  {
    std::chrono::nanoseconds ns(256 * 1000000);
    auto absolute = krbn::time_utility::to_absolute_time(ns);
    REQUIRE(krbn::time_utility::to_milliseconds(absolute) == std::chrono::milliseconds(256));
  }

  {
    std::chrono::milliseconds ms(256 * 1000000);
    auto absolute = krbn::time_utility::to_absolute_time(ms);
    REQUIRE(krbn::time_utility::to_milliseconds(absolute) == ms);
  }

  {
    krbn::absolute_time absolute(256 * 1000000);
    auto ns = krbn::time_utility::to_nanoseconds(absolute);
    REQUIRE(krbn::time_utility::to_absolute_time(ns) == absolute);
  }

  {
    krbn::absolute_time absolute(256 * 1000000);
    auto ms = krbn::time_utility::to_milliseconds(absolute);
    REQUIRE(krbn::time_utility::to_absolute_time(ms) == absolute);
  }
}
