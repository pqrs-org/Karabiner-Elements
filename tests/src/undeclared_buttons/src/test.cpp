#include "undeclared_buttons.hpp"
#include <boost/ut.hpp>

namespace {
using krbn::undeclared_buttons::button_change;

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

std::optional<krbn::undeclared_buttons::configuration> huge_plus_configuration() {
  return krbn::undeclared_buttons::find_configuration(
      pqrs::hid::vendor_id::value_t(0x056e),
      pqrs::hid::product_id::value_t(0x01aa),
      huge_plus_descriptor());
}

krbn::undeclared_buttons::decoder huge_plus_decoder() {
  return krbn::undeclared_buttons::decoder(*huge_plus_configuration());
}

std::vector<uint8_t> report(uint8_t buttons) {
  return {0x01, buttons, 0x00, 0x00, 0x00, 0x00};
}
} // namespace

int main() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "find HUGE PLUS configuration from its descriptor"_test = [] {
    auto configuration = huge_plus_configuration();
    expect(configuration != std::nullopt);
    expect(configuration->report_id == 1_u);
    expect(configuration->buttons_bit_offset == 13_u);
    expect(configuration->button_count == 3_u);
    expect(configuration->first_button == 6_u);
  };

  "find DEFT configuration from its descriptor"_test = [] {
    auto configuration = krbn::undeclared_buttons::find_configuration(
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
      expect(krbn::undeclared_buttons::find_configuration(
                 pqrs::hid::vendor_id::value_t(0x056e),
                 product_id,
                 huge_plus_descriptor()) != std::nullopt);
    }
  };

  "reject unknown device"_test = [] {
    expect(krbn::undeclared_buttons::find_configuration(
               pqrs::hid::vendor_id::value_t(0x056e),
               pqrs::hid::product_id::value_t(0x0001),
               huge_plus_descriptor()) == std::nullopt);
    expect(krbn::undeclared_buttons::find_configuration(
               pqrs::hid::vendor_id::value_t(0x05ac),
               pqrs::hid::product_id::value_t(0x01aa),
               huge_plus_descriptor()) == std::nullopt);
  };

  "reject changed descriptor"_test = [] {
    auto descriptor = huge_plus_descriptor();
    descriptor[17] = 0x08; // Firmware now declares all eight buttons.
    expect(krbn::undeclared_buttons::find_configuration(
               pqrs::hid::vendor_id::value_t(0x056e),
               pqrs::hid::product_id::value_t(0x01aa),
               descriptor) == std::nullopt);

    descriptor = huge_plus_descriptor();
    descriptor.resize(33);
    expect(krbn::undeclared_buttons::find_configuration(
               pqrs::hid::vendor_id::value_t(0x056e),
               pqrs::hid::product_id::value_t(0x01aa),
               descriptor) == std::nullopt);
  };

  "derive buttons from padding width"_test = [] {
    auto descriptor = huge_plus_descriptor();

    // Expand the Constant field after buttons 1-5 from 3 bits to 11 bits.
    // The recovered buttons therefore span two bytes and are numbered 6-16.
    descriptor[29] = 0x0b;

    auto configuration = krbn::undeclared_buttons::find_configuration(
        pqrs::hid::vendor_id::value_t(0x056e),
        pqrs::hid::product_id::value_t(0x01aa),
        descriptor);
    expect(configuration != std::nullopt);
    expect(configuration->buttons_bit_offset == 13_u);
    expect(configuration->button_count == 11_u);
    expect(configuration->first_button == 6_u);

    auto decoder = krbn::undeclared_buttons::decoder(*configuration);
    auto changes = decoder.update(1, std::vector<uint8_t>{0x01, 0x20, 0x81});
    expect(changes == std::vector<button_change>{
                          {.button = 6, .pressed = true},
                          {.button = 9, .pressed = true},
                          {.button = 16, .pressed = true},
                      });
  };

  "decode press, repeat and release"_test = [] {
    auto decoder = huge_plus_decoder();

    auto down = decoder.update(1, report(0x20));
    expect(down == std::vector<button_change>{{.button = 6, .pressed = true}});
    expect(decoder.update(1, report(0x20)).empty());

    auto up = decoder.update(1, report(0x00));
    expect(up == std::vector<button_change>{{.button = 6, .pressed = false}});
  };

  "decode recovered buttons only"_test = [] {
    auto decoder = huge_plus_decoder();

    // Declared button 1 is ignored; undeclared buttons 6 and 8 are reported.
    auto changes = decoder.update(1, report(0xa1));
    expect(changes == std::vector<button_change>{
                          {.button = 6, .pressed = true},
                          {.button = 8, .pressed = true},
                      });

    auto mixed = decoder.update(1, report(0x40));
    expect(mixed == std::vector<button_change>{
                        {.button = 6, .pressed = false},
                        {.button = 7, .pressed = true},
                        {.button = 8, .pressed = false},
                    });
  };

  "ignore unrelated or short report"_test = [] {
    auto decoder = huge_plus_decoder();
    expect(decoder.update(2, std::vector<uint8_t>{0x02, 0xff}).empty());
    expect(decoder.update(1, std::vector<uint8_t>{0x01}).empty());
    expect(decoder.update(1, std::vector<uint8_t>{}).empty());
  };

  "reset decoder"_test = [] {
    auto decoder = huge_plus_decoder();
    expect(decoder.update(1, report(0x20)).size() == 1_u);
    decoder.reset();
    expect(decoder.update(1, report(0x20)).size() == 1_u);
  };

  return 0;
}
