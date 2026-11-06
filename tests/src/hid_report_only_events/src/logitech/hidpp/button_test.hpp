#include "hid_report_only_events.hpp"
#include <boost/ut.hpp>

void run_logitech_hidpp_button_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "HID++ report handler registry"_test = [] {
    const krbn::device_identifiers identifiers({
        .vendor_id = pqrs::hid::vendor_id::value_t(0x046d),
        .product_id = pqrs::hid::product_id::value_t(0xb037),
        .is_pointing_device = true,
    });

    // A vendor-defined HID++ long report with 19-byte input and output payloads.
    // clang-format off
    const std::vector<uint8_t> report_descriptor{
        0x06, 0x43, 0xff, // Usage Page (Vendor-Defined 0xff43)
        0x0a, 0x02, 0x02, // Usage (0x0202)
        0xa1, 0x01,       // Collection (Application)
        0x85, 0x11,       //   Report ID (17, HID++ long report)
        0x75, 0x08,       //   Report Size (8 bits)
        0x95, 0x13,       //   Report Count (19)
        0x15, 0x00,       //   Logical Minimum (0)
        0x26, 0xff, 0x00, //   Logical Maximum (255)
        0x09, 0x02,       //   Usage (0x02)
        0x81, 0x00,       //   Input (Data, Array, Absolute): 19-byte HID++ report
        0x09, 0x02,       //   Usage (0x02)
        0x91, 0x00,       //   Output (Data, Array, Absolute): 19-byte HID++ report
        0xc0,             // End Collection
    };
    // clang-format on

    expect(krbn::hid_report_only_events::is_target_device(identifiers));
    auto handler = krbn::hid_report_only_events::make_report_handler(identifiers,
                                                                     report_descriptor);
    expect(std::dynamic_pointer_cast<krbn::hid_report_only_events::logitech::hidpp::button::handler>(handler) != nullptr);
  };

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
