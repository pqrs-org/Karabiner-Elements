#include <catch2/catch.hpp>

#include "connected_devices/connected_devices.hpp"

TEST_CASE("connected_devices::details::device") {
  {
    krbn::connected_devices::details::descriptions descriptions("manufacturer1",
                                                                "product1");
    krbn::device_identifiers identifiers(pqrs::hid::vendor_id::value_t(1234),
                                         pqrs::hid::product_id::value_t(5678),
                                         true,
                                         false);
    krbn::connected_devices::details::device device(descriptions,
                                                    identifiers,
                                                    true,
                                                    true,
                                                    true);

    REQUIRE(device.get_descriptions() == descriptions);
    REQUIRE(device.get_identifiers() == identifiers);
    REQUIRE(device.get_is_built_in_keyboard() == true);
    REQUIRE(device.get_is_built_in_trackpad() == true);
    REQUIRE(device.get_is_built_in_touch_bar() == true);

    REQUIRE(device.to_json() == nlohmann::json(
                                    {{
                                         "descriptions",
                                         {
                                             {
                                                 "manufacturer",
                                                 "manufacturer1",
                                             },
                                             {
                                                 "product",
                                                 "product1",
                                             },
                                         },
                                     },
                                     {
                                         "identifiers",
                                         {
                                             {
                                                 "vendor_id",
                                                 1234,
                                             },
                                             {
                                                 "product_id",
                                                 5678,
                                             },
                                             {
                                                 "is_keyboard",
                                                 true,
                                             },
                                             {
                                                 "is_pointing_device",
                                                 false,
                                             },
                                         },
                                     },
                                     {
                                         "is_built_in_keyboard",
                                         true,
                                     },
                                     {
                                         "is_built_in_trackpad",
                                         true,
                                     },
                                     {
                                         "is_built_in_touch_bar",
                                         true,
                                     }}));
  }
  {
    auto device1 = krbn::connected_devices::details::device::make_from_json(nlohmann::json(nullptr));
    auto device2 = krbn::connected_devices::details::device::make_from_json(nlohmann::json(
        {{
             "descriptions",
             {
                 {
                     "manufacturer",
                     "manufacturer2",
                 },
                 {
                     "product",
                     "product2",
                 },
             },
         },
         {
             "identifiers",
             {
                 {
                     "vendor_id",
                     1234,
                 },
                 {
                     "product_id",
                     5678,
                 },
                 {
                     "is_keyboard",
                     true,
                 },
                 {
                     "is_pointing_device",
                     false,
                 },
             },
         },
         {
             "is_built_in_keyboard",
             true,
         },
         {
             "is_built_in_trackpad",
             true,
         },
         {
             "is_built_in_touch_bar",
             true,
         }}));

    REQUIRE(device1.get_descriptions().get_manufacturer() == "");
    REQUIRE(device1.get_descriptions().get_product() == "");
    REQUIRE(device1.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
    REQUIRE(device1.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
    REQUIRE(device1.get_identifiers().get_is_keyboard() == false);
    REQUIRE(device1.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device1.get_is_built_in_keyboard() == false);
    REQUIRE(device1.get_is_built_in_trackpad() == false);
    REQUIRE(device1.get_is_built_in_touch_bar() == false);

    REQUIRE(device2.get_descriptions().get_manufacturer() == "manufacturer2");
    REQUIRE(device2.get_descriptions().get_product() == "product2");
    REQUIRE(device2.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
    REQUIRE(device2.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
    REQUIRE(device2.get_identifiers().get_is_keyboard() == true);
    REQUIRE(device2.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device2.get_is_built_in_keyboard() == true);
    REQUIRE(device2.get_is_built_in_trackpad() == true);
    REQUIRE(device2.get_is_built_in_touch_bar() == true);
  }

  // from device_properties

  {
    krbn::device_properties device_properties;
    krbn::connected_devices::details::device device(device_properties);

    REQUIRE(device.get_descriptions().get_manufacturer() == "");
    REQUIRE(device.get_descriptions().get_product() == "");
    REQUIRE(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(0));
    REQUIRE(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(0));
    REQUIRE(device.get_identifiers().get_is_keyboard() == false);
    REQUIRE(device.get_identifiers().get_is_pointing_device() == false);
    REQUIRE(device.get_is_built_in_keyboard() == false);
    REQUIRE(device.get_is_built_in_trackpad() == false);
    REQUIRE(device.get_is_built_in_touch_bar() == false);
  }
  {
    krbn::device_properties device_properties;
    device_properties
        .set_manufacturer("manufacturer")
        .set_product("product")
        .set(pqrs::hid::vendor_id::value_t(1234))
        .set(pqrs::hid::product_id::value_t(5678))
        .set_is_keyboard(true);

    {
      krbn::connected_devices::details::device device(device_properties);

      REQUIRE(device.get_descriptions().get_manufacturer() == "manufacturer");
      REQUIRE(device.get_descriptions().get_product() == "product");
      REQUIRE(device.get_identifiers().get_vendor_id() == pqrs::hid::vendor_id::value_t(1234));
      REQUIRE(device.get_identifiers().get_product_id() == pqrs::hid::product_id::value_t(5678));
      REQUIRE(device.get_identifiers().get_is_keyboard() == true);
      REQUIRE(device.get_identifiers().get_is_pointing_device() == false);
      REQUIRE(device.get_is_built_in_keyboard() == false);
      REQUIRE(device.get_is_built_in_trackpad() == false);
      REQUIRE(device.get_is_built_in_touch_bar() == false);
    }
  }
}
