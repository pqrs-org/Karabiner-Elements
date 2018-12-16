#include <catch2/catch.hpp>

#include "device_properties.hpp"

TEST_CASE("device_properties") {
  auto device_properties = krbn::device_properties(krbn::device_id(0),
                                                   nullptr);
  REQUIRE(device_properties.get_device_id());
  REQUIRE(*(device_properties.get_device_id()) == krbn::device_id(0));
}
