#include <catch2/catch.hpp>

#include "types.hpp"

TEST_CASE("system_preferences_keyboard_type json") {
  {
    pqrs::osx::iokit_hid_vendor_id vendor_id(1);
    pqrs::osx::iokit_hid_product_id product_id(2);
    pqrs::osx::iokit_hid_country_code country_code(3);
    pqrs::osx::iokit_keyboard_type keyboard_type(4);

    krbn::system_preferences_keyboard_type t1(vendor_id,
                                              product_id,
                                              country_code,
                                              keyboard_type);
    REQUIRE(t1.get_vendor_id() == vendor_id);
    REQUIRE(t1.get_product_id() == product_id);
    REQUIRE(t1.get_country_code() == country_code);
    REQUIRE(t1.get_keyboard_type() == keyboard_type);

    // to_json

    nlohmann::json json = t1;
    REQUIRE(json == nlohmann::json::object({
                        {"vendor_id", vendor_id},
                        {"product_id", product_id},
                        {"country_code", country_code},
                        {"keyboard_type", keyboard_type},
                    }));

    // from_json

    auto t2 = json.get<krbn::system_preferences_keyboard_type>();
    REQUIRE(t2.get_vendor_id() == vendor_id);
    REQUIRE(t2.get_product_id() == product_id);
    REQUIRE(t2.get_country_code() == country_code);
    REQUIRE(t2.get_keyboard_type() == keyboard_type);

    // operator==

    REQUIRE(t1 == t2);

    REQUIRE(t1 != krbn::system_preferences_keyboard_type(pqrs::osx::iokit_hid_vendor_id(0),
                                                         product_id,
                                                         country_code,
                                                         keyboard_type));
    REQUIRE(t1 != krbn::system_preferences_keyboard_type(vendor_id,
                                                         pqrs::osx::iokit_hid_product_id(0),
                                                         country_code,
                                                         keyboard_type));
    REQUIRE(t1 != krbn::system_preferences_keyboard_type(vendor_id,
                                                         product_id,
                                                         pqrs::osx::iokit_hid_country_code(0),
                                                         keyboard_type));
    REQUIRE(t1 != krbn::system_preferences_keyboard_type(vendor_id,
                                                         product_id,
                                                         country_code,
                                                         pqrs::osx::iokit_keyboard_type(0)));
  }
}
