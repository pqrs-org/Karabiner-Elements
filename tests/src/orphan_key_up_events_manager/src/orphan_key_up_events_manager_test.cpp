#include <catch2/catch.hpp>

#include "orphan_key_up_events_manager.hpp"

TEST_CASE("orphan_key_up_events_manager") {
  krbn::orphan_key_up_events_manager m;

  // ----------------------------------------
  // key_down, key_up

  m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
           krbn::event_type::key_down,
           krbn::absolute_time_point(1000));
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
           krbn::event_type::key_up,
           krbn::absolute_time_point(1010));
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  // ----------------------------------------
  // key_up, key_down, key_up, key_down, key_up

  m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
           krbn::event_type::key_up,
           krbn::absolute_time_point(1100));
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::b));

  m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
           krbn::event_type::key_down,
           krbn::absolute_time_point(1110));
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::b));

  m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
           krbn::event_type::key_up,
           krbn::absolute_time_point(1120));
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
           krbn::event_type::key_down,
           krbn::absolute_time_point(1130));
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  m.update(krbn::key_down_up_valued_event(krbn::key_code::b),
           krbn::event_type::key_up,
           krbn::absolute_time_point(1140));
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  // ----------------------------------------
  // ignore old event

  m.update(krbn::key_down_up_valued_event(krbn::key_code::c),
           krbn::event_type::key_up,
           krbn::absolute_time_point(900));
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  m.update(krbn::key_down_up_valued_event(krbn::key_code::c),
           krbn::event_type::key_up,
           krbn::absolute_time_point(1200));
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::c));

  // ----------------------------------------
  // clear

  m.clear();
  REQUIRE(m.find_orphan_key_up_event() == std::nullopt);

  m.update(krbn::key_down_up_valued_event(krbn::key_code::a),
           krbn::event_type::key_up,
           krbn::absolute_time_point(500));
  REQUIRE(m.find_orphan_key_up_event() == krbn::key_down_up_valued_event(krbn::key_code::a));
}
