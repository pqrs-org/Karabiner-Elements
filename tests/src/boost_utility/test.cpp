#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "boost_utility.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("signal2_combiner_call_while_true") {
  boost::signals2::signal<bool(void),
                          krbn::boost_utility::signal2_combiner_call_while_true>
      signal;

  int counter = 0;

  signal.connect([&]() {
    ++counter;
    return true;
  });

  signal.connect([&]() {
    ++counter;
    return true;
  });

  signal.connect([&]() {
    ++counter;
    return false;
  });

  signal.connect([&]() {
    // never called
    ++counter;
    return true;
  });

  REQUIRE(signal() == false);
  REQUIRE(counter == 3);
}
