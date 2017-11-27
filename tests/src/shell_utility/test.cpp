#define CATCH_CONFIG_MAIN
#include "../../vendor/catch/catch.hpp"

#include "shell_utility.hpp"
#include "thread_utility.hpp"

TEST_CASE("initialize") {
  krbn::thread_utility::register_main_thread();
}

TEST_CASE("make_background_command") {
  {
    std::string command = "open '/Applications/Utilities/Activity Monitor.app'";
    std::string expected = "open '/Applications/Utilities/Activity Monitor.app' &";
    REQUIRE(krbn::shell_utility::make_background_command(command) == expected);
  }
  {
    std::string command = "open '/Applications/Utilities/Activity Monitor.app'&";
    std::string expected = "open '/Applications/Utilities/Activity Monitor.app'&";
    REQUIRE(krbn::shell_utility::make_background_command(command) == expected);
  }
  {
    std::string command = "ls & echo hello";
    std::string expected = "ls & echo hello &";
    REQUIRE(krbn::shell_utility::make_background_command(command) == expected);
  }
}
