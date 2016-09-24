#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include <boost/optional/optional_io.hpp>
#include <ostream>

#include "spdlog_utility.hpp"

TEST_CASE("get_timestamp_number") {
  {
    auto actual = spdlog_utility::get_sort_key("[2016-09-22 20:18:37.649] [grabber] [info] version 0.90.36");
    REQUIRE(actual != boost::none);
    REQUIRE(*actual == 20160922201837649);
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
