#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "spdlog_utility.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>
#include <ostream>

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

int main(int argc, char* const argv[]) {
  thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
