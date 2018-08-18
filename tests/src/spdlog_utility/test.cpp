#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "spdlog_utility.hpp"
#include "thread_utility.hpp"
#include <boost/optional/optional_io.hpp>
#include <ostream>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("get_timestamp_number") {
  {
    auto actual = krbn::spdlog_utility::get_sort_key("[2016-10-15 00:09:47.283] [info] [grabber] version 0.90.50");
    REQUIRE(actual == 20161015000947283ULL);
  }
  {
    auto actual = krbn::spdlog_utility::get_sort_key("[]");
    REQUIRE(actual == boost::none);
  }
  {
    auto actual = krbn::spdlog_utility::get_sort_key("[yyyy-mm-dd hh:mm:ss.mmm]");
    REQUIRE(actual == boost::none);
  }
}

TEST_CASE("get_level") {
  {
    auto actual = krbn::spdlog_utility::get_level("[2016-10-15 00:09:47.283] [info] [grabber] version 0.90.50");
    REQUIRE(actual == spdlog::level::info);
  }
  {
    auto actual = krbn::spdlog_utility::get_level("[2016-10-15 00:09:47.283] [error] [grabber] version 0.90.50");
    REQUIRE(actual == spdlog::level::err);
  }
  {
    auto actual = krbn::spdlog_utility::get_level("[2016-10-15 00:09:47.283] [unknown] [grabber] version 0.90.50");
    REQUIRE(actual == boost::none);
  }
  {
    auto actual = krbn::spdlog_utility::get_level("[2016-10-15 00:09:47.283] ");
    REQUIRE(actual == boost::none);
  }
  {
    auto actual = krbn::spdlog_utility::get_level("[2016-10-15 00:09:47.283] [");
    REQUIRE(actual == boost::none);
  }
  {
    auto actual = krbn::spdlog_utility::get_level("[2016-10-15 00:09:47.283] [info");
    REQUIRE(actual == boost::none);
  }
  {
    auto actual = krbn::spdlog_utility::get_level("[2016-10-15 00:09:47.283] [info]");
    REQUIRE(actual == spdlog::level::info);
  }
}
