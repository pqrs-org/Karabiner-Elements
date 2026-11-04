#include "hid_report_only_events/logitech/hidpp/button.hpp"
#include <boost/ut.hpp>

void run_logitech_hidpp_button_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "HID++ logical button"_test = [] {
    const auto time_stamp = pqrs::osx::chrono::absolute_time_point(12345);

    krbn::hid_report_only_events::logitech::hidpp::button::handler handler;
    handler.set_input_report_handler(
        [](auto) {
          return std::vector<krbn::logitech::hidpp::session::result::button_event>{
              {.control_id = 196, .pressed = true},
          };
        });

    krbn::logitech::hidpp::long_report report{};
    report[0] = krbn::logitech::hidpp::long_report_id;
    report[1] = krbn::logitech::hidpp::direct_device_index;

    expect(handler.should_accept_report(krbn::logitech::hidpp::long_report_id, report));
    expect(!handler.should_accept_report(0, report));

    krbn::logitech::hidpp::short_report short_report{};
    short_report[0] = krbn::logitech::hidpp::short_report_id;
    short_report[1] = krbn::logitech::hidpp::direct_device_index;
    expect(handler.should_accept_report(krbn::logitech::hidpp::short_report_id, short_report));

    auto values = handler.handle(krbn::logitech::hidpp::long_report_id,
                                 report,
                                 time_stamp);
    expect(values == std::vector<pqrs::osx::iokit_hid_value>{
                         pqrs::osx::iokit_hid_value(
                             time_stamp,
                             1,
                             krbn::momentary_switch_event_details::logitech_hidpp_button::usage_page,
                             pqrs::hid::usage::value_t(196),
                             1,
                             0),
                     });

    size_t discovery_reports = 0;
    handler.set_input_report_handler(
        [&](auto) {
          ++discovery_reports;
          return std::vector<krbn::logitech::hidpp::session::result::button_event>{};
        });
    expect(handler.handle(krbn::logitech::hidpp::long_report_id,
                          report,
                          time_stamp)
               .empty());
    expect(discovery_reports == 1_u);

    handler.clear_input_report_handler();
    expect(handler.handle(krbn::logitech::hidpp::long_report_id,
                          report,
                          time_stamp)
               .empty());
  };
}
