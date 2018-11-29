#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "pressed_usage_manager.hpp"

TEST_CASE("pressed_usage_manager") {
  krbn::pressed_usage_manager manager;

  REQUIRE(manager.empty());

  manager.insert(pqrs::osx::iokit_hid_usage_page(1),
                 pqrs::osx::iokit_hid_usage(2));
  REQUIRE(!manager.empty());

  manager.insert(pqrs::osx::iokit_hid_usage_page(1),
                 pqrs::osx::iokit_hid_usage(2));
  REQUIRE(!manager.empty());

  manager.erase(pqrs::osx::iokit_hid_usage_page(1),
                pqrs::osx::iokit_hid_usage(3));
  REQUIRE(!manager.empty());

  manager.erase(pqrs::osx::iokit_hid_usage_page(1),
                pqrs::osx::iokit_hid_usage(2));
  REQUIRE(manager.empty());
}
