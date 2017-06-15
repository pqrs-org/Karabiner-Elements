#define CATCH_CONFIG_RUNNER
#include "../../vendor/catch/catch.hpp"

#include "boost_defs.hpp"

#include "spdlog_utility.hpp"
#include "thread_utility.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/optional/optional_io.hpp>
#include <ostream>

TEST_CASE("get_timestamp_number") {
  {
    auto actual = krbn::spdlog_utility::get_sort_key("[2016-10-15 00:09:47.283] [info] [grabber] version 0.90.50");
    REQUIRE(actual != boost::none);
    REQUIRE(*actual == 20161015000947283);
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

TEST_CASE("log_reducer") {
  krbn::spdlog_utility::log_reducer log_reducer;

  // reduce
  log_reducer.info("test1");
  log_reducer.info("test1");
  log_reducer.info("test1");
  log_reducer.warn("test1");
  log_reducer.warn("test1");
  log_reducer.warn("test1");
  log_reducer.error("test1");
  log_reducer.error("test1");
  log_reducer.error("test1");
  log_reducer.info("test1");
  log_reducer.warn("test1");
  log_reducer.error("test1");

  // reset
  log_reducer.info("test2");
  log_reducer.reset();
  log_reducer.info("test2");
  log_reducer.reset();
  log_reducer.info("test2");

  // old value will be removed after new items are pushed.
  log_reducer.info("test3");
  for (int i = 0; i < 100; ++i) {
    log_reducer.info(std::string("dummy ") + boost::lexical_cast<std::string>(i));
  }
  log_reducer.info("test3");
}

int main(int argc, char* const argv[]) {
  krbn::thread_utility::register_main_thread();
  return Catch::Session().run(argc, argv);
}
