#include <catch2/catch.hpp>

#include "connected_devices/connected_devices.hpp"

TEST_CASE("connected_devices::details::descriptions") {
  {
    krbn::connected_devices::details::descriptions descriptions1("manufacturer1",
                                                                 "product1");
    krbn::connected_devices::details::descriptions descriptions2("manufacturer2",
                                                                 "product2");
    krbn::connected_devices::details::descriptions descriptions3("manufacturer1",
                                                                 "product1");

    REQUIRE(descriptions1.get_manufacturer() == "manufacturer1");
    REQUIRE(descriptions1.get_product() == "product1");

    REQUIRE(descriptions1.to_json() == nlohmann::json({
                                           {"manufacturer", "manufacturer1"},
                                           {"product", "product1"},
                                       }));

    REQUIRE(descriptions1 == descriptions3);
    REQUIRE(descriptions1 != descriptions2);
  }
  {
    auto descriptions1 = krbn::connected_devices::details::descriptions::make_from_json(
        nlohmann::json(nullptr));
    auto descriptions2 = krbn::connected_devices::details::descriptions::make_from_json(
        nlohmann::json({
            {"manufacturer", "manufacturer2"},
            {"product", "product2"},
        }));

    REQUIRE(descriptions1.get_manufacturer() == "");
    REQUIRE(descriptions1.get_product() == "");

    REQUIRE(descriptions2.get_manufacturer() == "manufacturer2");
    REQUIRE(descriptions2.get_product() == "product2");
  }

  // from device_properties

  {
    krbn::device_properties device_properties;
    krbn::connected_devices::details::descriptions descriptions(device_properties);

    REQUIRE(descriptions.get_manufacturer() == "");
    REQUIRE(descriptions.get_product() == "");
  }
  {
    krbn::device_properties device_properties;
    device_properties
        .set_manufacturer("manufacturer")
        .set_product("product");
    krbn::connected_devices::details::descriptions descriptions(device_properties);

    REQUIRE(descriptions.get_manufacturer() == "manufacturer");
    REQUIRE(descriptions.get_product() == "product");
  }
}
