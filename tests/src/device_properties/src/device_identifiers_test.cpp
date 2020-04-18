#include <catch2/catch.hpp>

#include "device_properties.hpp"

TEST_CASE("device_identifiers") {
  {
    krbn::device_properties device_properties;

    auto di = device_properties.get_device_identifiers();
    REQUIRE(di);
    REQUIRE(di->get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
    REQUIRE(di->get_product_id() == pqrs::hid::product_id::value_t(0));
    REQUIRE(di->get_is_keyboard() == false);
    REQUIRE(di->get_is_pointing_device() == false);

    device_properties.set(pqrs::hid::vendor_id::value_t(1234));
    di = device_properties.get_device_identifiers();
    REQUIRE(di->get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));

    device_properties.set(pqrs::hid::product_id::value_t(5678));
    di = device_properties.get_device_identifiers();
    REQUIRE(di->get_product_id() == pqrs::hid::product_id::value_t(5678));

    device_properties.set_is_keyboard(true);
    di = device_properties.get_device_identifiers();
    REQUIRE(di->get_is_keyboard() == true);

    device_properties.set_is_pointing_device(true);
    di = device_properties.get_device_identifiers();
    REQUIRE(di->get_is_pointing_device() == true);
  }
}
