#include <catch2/catch.hpp>

#include "orphan_key_up_events_manager.hpp"

TEST_CASE("orphan_key_up_events_manager") {
  krbn::orphan_key_up_events_manager m;

  // ----------------------------------------
  // key_down, key_up

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
                   krbn::event_type::key_down,
                   krbn::absolute_time_point(1000)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(1010)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  // ----------------------------------------
  // key_up, key_down, key_up, key_down, key_up

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(1100)) == true);
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::b));

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
                   krbn::event_type::key_down,
                   krbn::absolute_time_point(1110)) == false);
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::b));

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(1120)) == true);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
                   krbn::event_type::key_down,
                   krbn::absolute_time_point(1130)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(1140)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  // ----------------------------------------
  // ignore old event

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::c),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(900)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::c),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(1200)) == true);
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::c));

  // ----------------------------------------
  // clear

  m.clear();
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(500)) == true);
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::a));
}

TEST_CASE("orphan_key_up_events_manager key_down is dropped") {
  krbn::orphan_key_up_events_manager m;

  // ----------------------------------------
  // key_down, key_up, key_up

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
                   krbn::event_type::key_down,
                   krbn::absolute_time_point(1000)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(1010)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  REQUIRE(m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
                   krbn::event_type::key_up,
                   krbn::absolute_time_point(1020)) == false);
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);
}
