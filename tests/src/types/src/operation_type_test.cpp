#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("json") {
  for (uint8_t i = static_cast<uint8_t>(krbn::operation_type::none);
       i < static_cast<uint8_t>(krbn::operation_type::end_);
       ++i) {
    auto t = krbn::operation_type(i);

    nlohmann::json json = t;
    REQUIRE(json.get<krbn::operation_type>() == t);
  }
}
