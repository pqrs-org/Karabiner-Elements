#include "event_tap_utility.hpp"
#include <boost/ut.hpp>

int main(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "make_event"_test = [] {
    {
      auto actual = krbn::event_tap_utility::make_event(kCGEventLeftMouseDown, nullptr);
      expect(actual->first == krbn::event_type::key_down);
      expect(actual->second == krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                     pqrs::hid::usage::button::button_1)));
    }
    {
      auto actual = krbn::event_tap_utility::make_event(kCGEventOtherMouseUp, nullptr);
      expect(actual->first == krbn::event_type::key_up);
      expect(actual->second == krbn::event_queue::event(krbn::momentary_switch_event(pqrs::hid::usage_page::button,
                                                                                     pqrs::hid::usage::button::button_3)));
    }
  };

  return 0;
}
