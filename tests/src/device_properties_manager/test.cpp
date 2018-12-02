#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "device_properties_manager.hpp"

TEST_CASE("device_properties_manager") {
  krbn::device_properties_manager manager;

  manager.insert(krbn::device_id(1),
                 krbn::device_properties(krbn::device_id(1), nullptr));
  manager.insert(krbn::device_id(2),
                 krbn::device_properties(krbn::device_id(2), nullptr));
  manager.insert(krbn::device_id(3),
                 std::make_shared<krbn::device_properties>(krbn::device_id(3), nullptr));

  // iokit_device_id(1)

  {
    auto dp = manager.find(krbn::device_id(1));
    REQUIRE(dp);
    REQUIRE(dp->get_device_id());
    REQUIRE(*(dp->get_device_id()) == krbn::device_id(1));
  }

  // iokit_device_id(2)

  {
    auto dp = manager.find(krbn::device_id(2));
    REQUIRE(dp);
    REQUIRE(dp->get_device_id());
    REQUIRE(*(dp->get_device_id()) == krbn::device_id(2));
  }

  // iokit_device_id(3)

  {
    auto dp = manager.find(krbn::device_id(3));
    REQUIRE(dp);
    REQUIRE(dp->get_device_id());
    REQUIRE(*(dp->get_device_id()) == krbn::device_id(3));
  }

  // iokit_device_id(4)

  {
    auto dp = manager.find(krbn::device_id(4));
    REQUIRE(!dp);
  }

  // erase iokit_device_id(2)

  {
    manager.erase(krbn::device_id(2));
    auto dp = manager.find(krbn::device_id(2));
    REQUIRE(!dp);
  }

  // clear

  {
    manager.clear();
    auto dp = manager.find(krbn::device_id(1));
    REQUIRE(!dp);
  }
}
