#include "undeclared_buttons.hpp"
#include <boost/ut.hpp>

namespace {
using krbn::undeclared_buttons::button_change;

std::vector<uint8_t> huge_plus_descriptor() {
  // clang-format off
  return {
      0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x01,
      0x09, 0x01, 0xa1, 0x00, 0x05, 0x09, 0x19, 0x01,
      0x29, 0x05, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01,
      0x95, 0x05, 0x81, 0x02, 0x75, 0x03, 0x95, 0x01,
      0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
      0x16, 0x01, 0x80, 0x26, 0xff, 0x7f, 0x75, 0x10,
      0x95, 0x02, 0x81, 0x06, 0xc0, 0xc0,
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

  "find known device configuration"_test = [] {
    auto c = huge_plus_configuration();
    expect(c != std::nullopt);
    expect(c->report_id == 1_u);
    expect(c->buttons_byte_index == 1_u);
    expect(c->button_mask == 0xe0);

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
