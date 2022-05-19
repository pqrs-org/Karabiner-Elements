#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("event_origin") {
  {
    auto event_origin = krbn::event_origin::grabbed_device;
    auto json = nlohmann::json(event_origin);
    REQUIRE(json.dump() == "\"grabbed_device\"");
  }
  {
    auto json = nlohmann::json("grabbed_device");
    REQUIRE(json.get<krbn::event_origin>() == krbn::event_origin::grabbed_device);
  }
}
