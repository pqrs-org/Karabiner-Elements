#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "logger.hpp"
#include "thread_utility.hpp"
#include <ostream>

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("unique_filter") {
  krbn::logger::unique_filter unique_filter;

  // reduce
  unique_filter.info("test1");
  unique_filter.info("test1");
  unique_filter.info("test1");
  unique_filter.warn("test1");
  unique_filter.warn("test1");
  unique_filter.warn("test1");
  unique_filter.error("test1");
  unique_filter.error("test1");
  unique_filter.error("test1");
  unique_filter.info("test1");
  unique_filter.warn("test1");
  unique_filter.error("test1");

  // reset
  unique_filter.info("test2");
  unique_filter.reset();
  unique_filter.info("test2");
  unique_filter.reset();
  unique_filter.info("test2");

  // old value will be removed after new items are pushed.
  unique_filter.info("test3");
  for (int i = 0; i < 100; ++i) {
    unique_filter.info(std::string("dummy ") + boost::lexical_cast<std::string>(i));
  }
  unique_filter.info("test3");
}
