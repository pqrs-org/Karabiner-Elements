#include "hid_report_only_events/elecom/trackball.hpp"
#include <boost/ut.hpp>

namespace {
namespace elecom_trackball = krbn::hid_report_only_events::elecom::trackball;

const auto time_stamp = pqrs::osx::chrono::absolute_time_point(12345);

std::vector<uint8_t> huge_plus_descriptor() {
  // Mouse Application Collection extracted from the
  // ELECOM HUGE PLUS (056e:01aa) report descriptor.
  // clang-format off
  return {
      0x05, 0x01,       // Usage Page (Generic Desktop)
      0x09, 0x02,       // Usage (Mouse)
      0xa1, 0x01,       // Collection (Application)
      0x85, 0x01,       //   Report ID (1)
      0x09, 0x01,       //   Usage (Pointer)
      0xa1, 0x00,       //   Collection (Physical)
      0x05, 0x09,       //     Usage Page (Button)
      0x19, 0x01,       //     Usage Minimum (Button 1)
      0x29, 0x05,       //     Usage Maximum (Button 5)
      0x15, 0x00,       //     Logical Minimum (0)
      0x25, 0x01,       //     Logical Maximum (1)
      0x75, 0x01,       //     Report Size (1 bit)
      0x95, 0x05,       //     Report Count (5)
      0x81, 0x02,       //     Input (Data, Variable, Absolute): buttons 1-5
      0x75, 0x03,       //     Report Size (3 bits)
      0x95, 0x01,       //     Report Count (1)
      0x81, 0x01,       //     Input (Constant): storage used by buttons 6-8
      0x05, 0x01,       //     Usage Page (Generic Desktop)
      0x09, 0x30,       //     Usage (X)
      0x09, 0x31,       //     Usage (Y)
      0x16, 0x01, 0x80, //     Logical Minimum (-32767)
      0x26, 0xff, 0x7f, //     Logical Maximum (32767)
      0x75, 0x10,       //     Report Size (16 bits)
      0x95, 0x02,       //     Report Count (2)
      0x81, 0x06,       //     Input (Data, Variable, Relative): X, Y
      0xc0,             //   End Collection
      0xc0,             // End Collection
  };
  // clang-format on
}

std::vector<uint8_t> deft_descriptor() {
  // Mouse Application Collection extracted from the
  // ELECOM DEFT (056e:00fe) report descriptor.
  // Unlike HUGE PLUS, the Report ID is declared after entering the Pointer
  // collection.
  // clang-format off
  return {
      0x05, 0x01,       // Usage Page (Generic Desktop)
      0x09, 0x02,       // Usage (Mouse)
      0xa1, 0x01,       // Collection (Application)
      0x09, 0x01,       //   Usage (Pointer)
      0xa1, 0x00,       //   Collection (Physical)
      0x85, 0x01,       //     Report ID (1)
      0x95, 0x05,       //     Report Count (5)
      0x75, 0x01,       //     Report Size (1 bit)
      0x05, 0x09,       //     Usage Page (Button)
      0x19, 0x01,       //     Usage Minimum (Button 1)
      0x29, 0x05,       //     Usage Maximum (Button 5)
      0x15, 0x00,       //     Logical Minimum (0)
      0x25, 0x01,       //     Logical Maximum (1)
      0x81, 0x02,       //     Input (Data, Variable, Absolute): buttons 1-5
      0x95, 0x01,       //     Report Count (1)
      0x75, 0x03,       //     Report Size (3 bits)
      0x81, 0x01,       //     Input (Constant): storage used by buttons 6-8
      0x75, 0x10,       //     Report Size (16 bits)
      0x95, 0x02,       //     Report Count (2)
      0x05, 0x01,       //     Usage Page (Generic Desktop)
      0x09, 0x30,       //     Usage (X)
      0x09, 0x31,       //     Usage (Y)
      0x16, 0x00, 0x80, //     Logical Minimum (-32768)
      0x26, 0xff, 0x7f, //     Logical Maximum (32767)
      0x81, 0x06,       //     Input (Data, Variable, Relative): X, Y
      0xc0,             //   End Collection
      0xc0,             // End Collection
  };
  // clang-format on
}

std::optional<elecom_trackball::configuration> huge_plus_configuration() {
  return elecom_trackball::find_configuration(
      pqrs::hid::vendor_id::value_t(0x056e),
      pqrs::hid::product_id::value_t(0x01aa),
      huge_plus_descriptor());
}

elecom_trackball::handler huge_plus_handler() {
  return elecom_trackball::handler(*huge_plus_configuration());
}

std::vector<uint8_t> report(uint8_t buttons) {
  return {0x01, buttons, 0x00, 0x00, 0x00, 0x00};
}

pqrs::osx::iokit_hid_value button_value(uint32_t button, bool pressed) {
  return pqrs::osx::iokit_hid_value(
      time_stamp,
      pressed ? 1 : 0,
      pqrs::hid::usage_page::button,
      pqrs::hid::usage::value_t(button),
      1,
      0);
}
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "identify target devices"_test = [] {
    for (auto product_id : {
             pqrs::hid::product_id::value_t(0x00fe),
             pqrs::hid::product_id::value_t(0x010c),
             pqrs::hid::product_id::value_t(0x010d),
             pqrs::hid::product_id::value_t(0x011c),
             pqrs::hid::product_id::value_t(0x01aa),
             pqrs::hid::product_id::value_t(0x01ab),
             pqrs::hid::product_id::value_t(0x01ac),
         }) {
      expect(elecom_trackball::is_target_device(
          pqrs::hid::vendor_id::value_t(0x056e),
          product_id));
    }

    expect(!elecom_trackball::is_target_device(
        pqrs::hid::vendor_id::value_t(0x056e),
        pqrs::hid::product_id::value_t(0x0001)));
    expect(!elecom_trackball::is_target_device(
        pqrs::hid::vendor_id::value_t(0x05ac),
        pqrs::hid::product_id::value_t(0x01aa)));
  };

  "find HUGE PLUS configuration from its descriptor"_test = [] {
    auto configuration = huge_plus_configuration();
    expect(configuration != std::nullopt);
    expect(configuration->report_id == 1_u);
    expect(configuration->buttons_bit_offset == 13_u);
    expect(configuration->button_count == 3_u);
    expect(configuration->first_button == 6_u);
  };

  "find DEFT configuration from its descriptor"_test = [] {
    auto configuration = elecom_trackball::find_configuration(
        pqrs::hid::vendor_id::value_t(0x056e),
        pqrs::hid::product_id::value_t(0x00fe),
        deft_descriptor());
    expect(configuration != std::nullopt);
    expect(configuration->report_id == 1_u);
    expect(configuration->buttons_bit_offset == 13_u);
    expect(configuration->button_count == 3_u);
    expect(configuration->first_button == 6_u);
  };

  "accept HUGE PLUS product family"_test = [] {
    for (auto product_id : {
             pqrs::hid::product_id::value_t(0x01aa),
             pqrs::hid::product_id::value_t(0x01ab),
             pqrs::hid::product_id::value_t(0x01ac),
         }) {
      expect(elecom_trackball::find_configuration(
                 pqrs::hid::vendor_id::value_t(0x056e),
                 product_id,
                 huge_plus_descriptor()) != std::nullopt);
    }
  };

  "reject unknown device"_test = [] {
    expect(elecom_trackball::find_configuration(
               pqrs::hid::vendor_id::value_t(0x056e),
               pqrs::hid::product_id::value_t(0x0001),
               huge_plus_descriptor()) == std::nullopt);
    expect(elecom_trackball::find_configuration(
               pqrs::hid::vendor_id::value_t(0x05ac),
               pqrs::hid::product_id::value_t(0x01aa),
               huge_plus_descriptor()) == std::nullopt);
  };

  "reject changed descriptor"_test = [] {
    auto descriptor = huge_plus_descriptor();
    descriptor[17] = 0x08; // Firmware now declares all eight buttons.
    expect(elecom_trackball::find_configuration(
               pqrs::hid::vendor_id::value_t(0x056e),
               pqrs::hid::product_id::value_t(0x01aa),
               descriptor) == std::nullopt);

    descriptor = huge_plus_descriptor();
    descriptor.resize(33);
    expect(elecom_trackball::find_configuration(
               pqrs::hid::vendor_id::value_t(0x056e),
               pqrs::hid::product_id::value_t(0x01aa),
               descriptor) == std::nullopt);
  };

  "derive buttons from padding width"_test = [] {
    auto descriptor = huge_plus_descriptor();

    // Expand the Constant field after buttons 1-5 from 3 bits to 11 bits.
    // The recovered buttons therefore span two bytes and are numbered 6-16.
    descriptor[29] = 0x0b;

    auto configuration = elecom_trackball::find_configuration(
        pqrs::hid::vendor_id::value_t(0x056e),
        pqrs::hid::product_id::value_t(0x01aa),
        descriptor);
    expect(configuration != std::nullopt);
    expect(configuration->buttons_bit_offset == 13_u);
    expect(configuration->button_count == 11_u);
    expect(configuration->first_button == 6_u);

    auto handler = elecom_trackball::handler(*configuration);
    auto values = handler.handle(1,
                                 std::vector<uint8_t>{0x01, 0x20, 0x81},
                                 time_stamp);
    expect(values == std::vector<pqrs::osx::iokit_hid_value>{
                         button_value(6, true),
                         button_value(9, true),
                         button_value(16, true),
                     });
  };

  "handle press, repeat and release"_test = [] {
    auto handler = huge_plus_handler();

    auto down = handler.handle(1, report(0x20), time_stamp);
    expect(down == std::vector<pqrs::osx::iokit_hid_value>{button_value(6, true)});
    expect(handler.handle(1, report(0x20), time_stamp).empty());

    auto up = handler.handle(1, report(0x00), time_stamp);
    expect(up == std::vector<pqrs::osx::iokit_hid_value>{button_value(6, false)});
  };

  "match applicable reports"_test = [] {
    auto handler = huge_plus_handler();

    expect(handler.should_accept_report(1, report(0x00)));
    expect(!handler.should_accept_report(2, report(0x00)));
    expect(!handler.should_accept_report(1, std::vector<uint8_t>{0x01}));
    expect(!handler.should_accept_report(1, std::vector<uint8_t>{}));
  };

  "handle recovered buttons only"_test = [] {
    auto handler = huge_plus_handler();

    // Declared button 1 is ignored; undeclared buttons 6 and 8 are reported.
    auto values = handler.handle(1, report(0xa1), time_stamp);
    expect(values == std::vector<pqrs::osx::iokit_hid_value>{
                         button_value(6, true),
                         button_value(8, true),
                     });

    auto mixed = handler.handle(1, report(0x40), time_stamp);
    expect(mixed == std::vector<pqrs::osx::iokit_hid_value>{
                        button_value(6, false),
                        button_value(7, true),
                        button_value(8, false),
                    });
  };

  "reset handler"_test = [] {
    auto handler = huge_plus_handler();
    expect(handler.handle(1, report(0x20), time_stamp).size() == 1_u);
    handler.reset();
    expect(handler.handle(1, report(0x20), time_stamp).size() == 1_u);
  };

  return 0;
}
