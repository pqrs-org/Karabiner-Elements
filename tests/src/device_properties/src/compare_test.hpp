#include "device_properties.hpp"
#include <boost/ut.hpp>

void run_compare_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "compare"_test = [] {
    using namespace std::string_literals;

    krbn::device_properties device_properties0;

    krbn::device_properties device_properties1(krbn::device_properties::initialization_parameters{
        .device_id = krbn::device_id(98765),
        .vendor_id = pqrs::hid::vendor_id::value_t(123),
        .product_id = pqrs::hid::product_id::value_t(234),
        .location_id = krbn::location_id(345),
        .manufacturer = pqrs::hid::manufacturer_string::value_t("m1"),
        .product = pqrs::hid::product_string::value_t("p1"),
        .serial_number = "s1"s,
        .transport = "t1"s,
        .is_keyboard = true,
        .is_pointing_device = true,
    });

    krbn::device_properties device_properties2(krbn::device_properties::initialization_parameters{
        .device_id = krbn::device_id(98765),
        .vendor_id = pqrs::hid::vendor_id::value_t(123),
        .product_id = pqrs::hid::product_id::value_t(234),
        .location_id = krbn::location_id(345),
        .manufacturer = pqrs::hid::manufacturer_string::value_t("m2"),
        .product = pqrs::hid::product_string::value_t("p1"),
        .serial_number = "s1"s,
        .transport = "t1"s,
        .is_keyboard = true,
    });

    krbn::device_properties device_properties3(krbn::device_properties::initialization_parameters{
        .device_id = krbn::device_id(98765),
        .vendor_id = pqrs::hid::vendor_id::value_t(123),
        .product_id = pqrs::hid::product_id::value_t(234),
        .location_id = krbn::location_id(345),
        .manufacturer = pqrs::hid::manufacturer_string::value_t("m1"),
        .product = pqrs::hid::product_string::value_t("p2"),
        .serial_number = "s1"s,
        .transport = "t1"s,
        .is_pointing_device = true,
    });

    krbn::device_properties device_properties4(krbn::device_properties::initialization_parameters{
        .device_id = krbn::device_id(98765),
        .vendor_id = pqrs::hid::vendor_id::value_t(123),
        .product_id = pqrs::hid::product_id::value_t(234),
        .location_id = krbn::location_id(345),
        .manufacturer = pqrs::hid::manufacturer_string::value_t("m2"),
        .product = pqrs::hid::product_string::value_t("p2"),
        .serial_number = "s1"s,
        .transport = "t1"s,
    });

    expect(device_properties0.compare(device_properties0) == false);
    expect(device_properties1.compare(device_properties1) == false);
    // 0,1 (["",""], [p1,m1])
    expect(device_properties1.compare(device_properties0) == false);
    expect(device_properties0.compare(device_properties1) == true);
    // 1,2 ([p1,m1], [p1,m2])
    expect(device_properties1.compare(device_properties2) == true);
    expect(device_properties2.compare(device_properties1) == false);
    // 1,3 ([p1,m1], [p2,m1])
    expect(device_properties1.compare(device_properties3) == true);
    expect(device_properties3.compare(device_properties1) == false);
    // 3,4 ([p2,m1], [p2,m2])
    expect(device_properties3.compare(device_properties4) == true);
    expect(device_properties4.compare(device_properties3) == false);
  };
}
