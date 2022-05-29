#include "manipulator/manipulators/mouse_motion_to_scroll/options.hpp"
#include <boost/ut.hpp>
#include <iostream>

void run_options_test(void) {
  namespace mouse_motion_to_scroll = krbn::manipulator::manipulators::mouse_motion_to_scroll;

  "options"_test = [] {
    using options = mouse_motion_to_scroll::options;

    {
      options p;

      // speed_multiplier_

      expect(p.get_speed_multiplier() == 1.00_d);

      p.set_speed_multiplier(-100); // Should be ignored
      expect(p.get_speed_multiplier() == 1.00_d);

      p.set_speed_multiplier(0); // Should be ignored
      expect(p.get_speed_multiplier() == 1.00_d);

      p.set_speed_multiplier(0.5);
      expect(p.get_speed_multiplier() == 0.50_d);

      p.set_speed_multiplier(1.5);
      expect(p.get_speed_multiplier() == 1.50_d);

      // recent_time_duration_milliseconds_

      p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(-100));
      expect(p.get_recent_time_duration_milliseconds() == options::recent_time_duration_milliseconds_default_value);

      p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(0));
      expect(p.get_recent_time_duration_milliseconds() == options::recent_time_duration_milliseconds_default_value);

      p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(1));
      expect(p.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(1));

      p.set_recent_time_duration_milliseconds(std::chrono::milliseconds(200));
      expect(p.get_recent_time_duration_milliseconds() == std::chrono::milliseconds(200));

      // threshold_

      p.set_threshold(-100);
      expect(p.get_threshold() == options::threshold_default_value);

      p.set_threshold(0);
      expect(p.get_threshold() == options::threshold_default_value);

      p.set_threshold(1);
      expect(p.get_threshold() == 1);

      p.set_threshold(1000);
      expect(p.get_threshold() == 1000);

      // direction_lock_threshold_

      p.set_direction_lock_threshold(-100);
      expect(p.get_direction_lock_threshold() == options::direction_lock_threshold_default_value);

      p.set_direction_lock_threshold(0);
      expect(p.get_direction_lock_threshold() == options::direction_lock_threshold_default_value);

      p.set_direction_lock_threshold(1);
      expect(p.get_direction_lock_threshold() == 1);

      p.set_direction_lock_threshold(1000);
      expect(p.get_direction_lock_threshold() == 1000);

      // scroll_event_interval_milliseconds_threshold_

      p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(-100));
      expect(p.get_scroll_event_interval_milliseconds_threshold() == options::scroll_event_interval_milliseconds_threshold_default_value);

      p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(0));
      expect(p.get_scroll_event_interval_milliseconds_threshold() == options::scroll_event_interval_milliseconds_threshold_default_value);

      p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(1));
      expect(p.get_scroll_event_interval_milliseconds_threshold() == std::chrono::milliseconds(1));

      p.set_scroll_event_interval_milliseconds_threshold(std::chrono::milliseconds(1000));
      expect(p.get_scroll_event_interval_milliseconds_threshold() == std::chrono::milliseconds(1000));
    }
  };
}
