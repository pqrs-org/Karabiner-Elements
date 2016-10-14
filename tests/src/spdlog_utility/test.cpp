#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include <boost/optional/optional_io.hpp>
#include <ostream>

#include "spdlog_utility.hpp"

TEST_CASE("get_timestamp_number") {
  {
    auto actual = spdlog_utility::get_sort_key("[2016-10-15 00:09:47.283] [info] [grabber] version 0.90.50");
    REQUIRE(actual != boost::none);
    REQUIRE(*actual == 20161015000947283);
  }
  {
    auto actual = spdlog_utility::get_sort_key("[]");
    REQUIRE(actual == boost::none);
  }
  {
    auto actual = spdlog_utility::get_sort_key("[yyyy-mm-dd hh:mm:ss.mmm]");
    REQUIRE(actual == boost::none);
  }
}
