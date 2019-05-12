#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("grabbable_state") {
  {
    krbn::grabbable_state grabbable_state;
    REQUIRE(grabbable_state.get_device_id() == krbn::device_id(0));
    REQUIRE(grabbable_state.get_state() == krbn::grabbable_state::state::grabbable);
    REQUIRE(grabbable_state.get_time_stamp() == krbn::absolute_time_point(0));
  }
  {
    krbn::grabbable_state grabbable_state(krbn::device_id(1234),
                                          krbn::grabbable_state::state::ungrabbable_temporarily,
                                          krbn::absolute_time_point(1000));
    REQUIRE(grabbable_state.get_device_id() == krbn::device_id(1234));
    REQUIRE(grabbable_state.get_state() == krbn::grabbable_state::state::ungrabbable_temporarily);
    REQUIRE(grabbable_state.get_time_stamp() == krbn::absolute_time_point(1000));
  }
}

TEST_CASE("grabbable_state::equals_except_time_stamp") {
  {
    krbn::grabbable_state grabbable_state1(krbn::device_id(1234),
                                           krbn::grabbable_state::state::ungrabbable_temporarily,
                                           krbn::absolute_time_point(1000));
    krbn::grabbable_state grabbable_state2(krbn::device_id(1234),
                                           krbn::grabbable_state::state::ungrabbable_temporarily,
                                           krbn::absolute_time_point(2000));
    krbn::grabbable_state grabbable_state3(krbn::device_id(1234),
                                           krbn::grabbable_state::state::device_error,
                                           krbn::absolute_time_point(3000));
    REQUIRE(grabbable_state1 != grabbable_state2);
    REQUIRE(grabbable_state1.equals_except_time_stamp(grabbable_state2));
    REQUIRE(!grabbable_state1.equals_except_time_stamp(grabbable_state3));
  }
}

TEST_CASE("grabbable_state json") {
  for (uint8_t i = static_cast<uint8_t>(krbn::grabbable_state::state::none);
       i < static_cast<uint8_t>(krbn::grabbable_state::state::end_);
       ++i) {
    auto t = krbn::grabbable_state::state(i);

    nlohmann::json json = t;
    REQUIRE(json.get<krbn::grabbable_state::state>() == t);
  }

  {
    krbn::grabbable_state grabbable_state1(krbn::device_id(1234),
                                           krbn::grabbable_state::state::ungrabbable_temporarily,
                                           krbn::absolute_time_point(1000));
    nlohmann::json json = grabbable_state1;
    auto grabbable_state2 = json.get<krbn::grabbable_state>();
    REQUIRE(grabbable_state1 == grabbable_state2);
  }
  {
    nlohmann::json json;
    auto grabbable_state = json.get<krbn::grabbable_state>();
    REQUIRE(grabbable_state.get_device_id() == krbn::device_id(0));
    REQUIRE(grabbable_state.get_state() == krbn::grabbable_state::state::grabbable);
    REQUIRE(grabbable_state.get_time_stamp() == krbn::absolute_time_point(0));
  }
}
